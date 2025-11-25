#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
LOG="$ROOT_DIR/bot.log"

echo "Stopping existing bot processes..."
PIDS=$(pgrep -f "$ROOT_DIR/bot" || true)
if [ -n "$PIDS" ]; then
  echo "Killing: $PIDS"
  kill $PIDS || true
  sleep 1
fi

if [ -z "${BOT_TOKEN-}" ]; then
  echo "Warning: BOT_TOKEN is not set. Bot will not be able to send messages to Telegram." >&2
fi

PORT=${PORT:-10000}
echo "Starting bot on port $PORT (logs => $LOG)"
nohup "$ROOT_DIR/bot" > "$LOG" 2>&1 &
echo $! > "$ROOT_DIR/bot.pid"
echo "Started (pid=$(cat $ROOT_DIR/bot.pid))"
#!/usr/bin/env bash
set -euo pipefail

# run.sh: безопасный запуск бота — остановит существующий процесс, запустит новый
# Usage: BOT_TOKEN=... ./run.sh [PORT]

PORT=${1:-${PORT:-10000}}
BOT_TOKEN_ARG=${BOT_TOKEN:-""}
if [ -z "$BOT_TOKEN_ARG" ]; then
  if [ -n "${1:-}" ]; then
    # if first arg looks like token and BOT_TOKEN not set, support positional
    BOT_TOKEN_ARG=$1
  fi
fi

if [ -z "$BOT_TOKEN_ARG" ]; then
  echo "Usage: BOT_TOKEN=your_token ./run.sh [PORT]" >&2
  exit 1
fi

echo "Using PORT=$PORT"

# find process listening on the port and kill it
OLD_PID=$(ss -ltnp 2>/dev/null | awk -v p=$PORT '$4 ~ ":"p"$" {print $NF}' | sed -n 's/.*pid=\([0-9]*\).*/\1/p' | head -n1 || true)
if [ -n "$OLD_PID" ]; then
  echo "Killing existing process on port $PORT: PID $OLD_PID"
  kill "$OLD_PID" || true
  sleep 1
fi

export BOT_TOKEN="$BOT_TOKEN_ARG"
export PORT="$PORT"

nohup ./bot > bot.log 2>&1 &
NEWPID=$!
echo "Started bot (PID $NEWPID), logs -> bot.log"
