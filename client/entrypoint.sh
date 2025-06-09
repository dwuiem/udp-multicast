#!/bin/sh
set -e

# Получаем IP-адрес сервера по имени контейнера из переменной окружения $SERVER
SERVER_IP=$(ping -c1 "$SERVER" | grep PING | awk '{print $3}' | tr -d '()')

# Разрешаем UDP-трафик от IP-адреса сервера на порт 12345
iptables -A INPUT -s "$SERVER_IP" -p udp --dport 12345 -j ACCEPT

# Блокируем всё остальное входящее соединение
iptables -A INPUT -j DROP

./client
