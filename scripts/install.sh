#!/bin/bash
set -e

echo "=== УСТАНОВКА IMMUTABLE CONTAINER GUARD ==="

echo "1. Сборка программы..."
cd ~/kursovaya
make

echo "2. Установка программы..."
sudo make install

echo "3. Настройка пользователя..."
if ! getent passwd user-12-32 > /dev/null 2>&1; then
    sudo useradd -r -s /sbin/nologin -d /var/lib/immutable-guard user-12-32
fi

echo "4. Настройка директорий..."
sudo mkdir -p /var/lib/immutable-guard/hashes
sudo chown -R user-12-32:user-12-32 /var/lib/immutable-guard

echo "5. Настройка systemd..."
sudo systemctl daemon-reload

echo "=== УСТАНОВКА ЗАВЕРШЕНА ==="
echo "Команды для проверки:"
echo "  sudo systemctl start immutable-guard"
echo "  curl http://localhost:8080/health"
