FROM ubuntu:24.04

WORKDIR /app

# Встановлення залежностей
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Копіювання файлів проекту
COPY . .

# Виставимо змінну PORT (Render задає свій PORT автоматично)
ENV PORT=10000

# Компіляція бота
RUN make

# Публікуємо порт
EXPOSE 10000

# Запуск бота
CMD ["./bot"]
