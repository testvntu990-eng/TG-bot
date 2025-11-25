#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using json = nlohmann::json;
using namespace std;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void sendMessage(long long chat_id, const string &text) {
    const char* tok = getenv("BOT_TOKEN");
    if (!tok) return;
    string API_TOKEN(tok);
    string BASE_URL = string("https://api.telegram.org/bot") + API_TOKEN;

    CURL* curl = curl_easy_init();
    if (!curl) return;

    string url = BASE_URL + "/sendMessage";
    json payload;
    payload["chat_id"] = chat_id;
    payload["text"] = text;

    string json_str = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void handleUpdate(const json &upd) {
    try {
        if (upd.contains("message")) {
            json msg = upd["message"];
            string text = msg.contains("text") ? msg["text"].get<string>() : string();
            long long chat_id = 0;
            if (msg.contains("chat") && msg["chat"].contains("id")) chat_id = msg["chat"]["id"].get<long long>();
            cout << "[UPDATE] chat=" << chat_id << " text='" << text << "'\n";
            if (chat_id != 0 && !text.empty()) {
                sendMessage(chat_id, string("Echo: ") + text);
            }
        } else if (upd.contains("callback_query")) {
            auto cb = upd["callback_query"];
            string data = cb.value("data", "");
            long long user_id = 0;
            if (cb.contains("from") && cb["from"].contains("id")) user_id = cb["from"]["id"].get<long long>();
            cout << "[CALLBACK] from=" << user_id << " data='" << data << "'\n";
        } else {
            cout << "[UPDATE] (unknown) " << upd.dump() << "\n";
        }
    } catch (const exception &e) {
        cerr << "handleUpdate exception: " << e.what() << "\n";
    }
}

void httpServer(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { cerr << "socket failed\n"; return; }
    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) { cerr << "bind failed\n"; close(server_fd); return; }
    if (listen(server_fd, 16) == -1) { cerr << "listen failed\n"; close(server_fd); return; }
    cout << "[HTTP] listening on port " << port << "\n";

    while (true) {
        struct sockaddr_in client_addr; socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) continue;
        string req; char buf[8192]; ssize_t n = recv(client_fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) { close(client_fd); continue; }
        buf[n] = '\0'; req.assign(buf, n);
        size_t pos_line = req.find("\r\n"); string request_line = pos_line!=string::npos?req.substr(0,pos_line):req;
        istringstream iss(request_line); string method, path; iss>>method>>path;
        int content_length = 0; size_t pos_cl = req.find("Content-Length:");
        if (pos_cl!=string::npos) { size_t eol=req.find('\n', pos_cl); string cl=req.substr(pos_cl, eol-pos_cl); try{ size_t c=cl.find(":"); if (c!=string::npos) content_length=stoi(cl.substr(c+1)); }catch(...){} }
        string body; size_t pos_body=req.find("\r\n\r\n"); if (pos_body!=string::npos) body=req.substr(pos_body+4);
        while ((int)body.size() < content_length) { ssize_t m = recv(client_fd, buf, sizeof(buf)-1, 0); if (m<=0) break; buf[m]='\0'; body.append(buf, m); }

        if (method=="GET" && (path=="/"||path=="/health")) {
            string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK";
            send(client_fd, resp.c_str(), resp.size(), 0); close(client_fd); continue;
        }
        if (method=="POST" && path.rfind("/webhook",0)==0) {
            try { json upd = json::parse(body); thread t([upd]() mutable { handleUpdate(upd); }); t.detach(); } catch(...) {}
            string resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"ok\"}";
            send(client_fd, resp.c_str(), resp.size(), 0); close(client_fd); continue;
        }
        string resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nNot Found";
        send(client_fd, resp.c_str(), resp.size(), 0); close(client_fd);
    }

    close(server_fd);
}

// set webhook via Telegram API (simple GET call)
void setWebhook(const string &token, const string &webhook_url) {
    if (token.empty() || webhook_url.empty()) return;
    string url = string("https://api.telegram.org/bot") + token + "/setWebhook?url=" + webhook_url;
    CURL* curl = curl_easy_init();
    if (!curl) { cerr<<"setWebhook: curl init failed"<<"\n"; return; }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    string resp;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        cerr << "setWebhook failed: " << curl_easy_strerror(rc) << "\n";
    } else {
        cout << "setWebhook response: " << resp << "\n";
    }
    curl_easy_cleanup(curl);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    const char* port_env = getenv("PORT"); int port = 10000; if (port_env) try{ port=stoi(string(port_env)); }catch(...){}

    // attempt to set webhook automatically if variables are present
    const char* tok = getenv("BOT_TOKEN");
    const char* wh = getenv("WEBHOOK_URL");
    if (tok && wh && strlen(tok)>0 && strlen(wh)>0) {
        try {
            setWebhook(string(tok), string(wh));
        } catch(...) { cerr<<"setWebhook exception"<<"\n"; }
    }

    thread http_thread(httpServer, port); http_thread.detach();
    cout<<"=== BOT STARTED ===\n";
    while (true) pause();
    curl_global_cleanup();
    return 0;
}
