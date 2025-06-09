#!/bin/sh
set -e

# Разделяем список клиентов (например, client1,client2...) на отдельные имена
for client in $(echo "$CLIENTS" | tr ',' ' '); do
    # Получаем IP-адрес каждого клиента
    CLIENT_IP=$(ping -c1 "$client" | grep PING | awk '{print $3}' | tr -d '()')

    # Разрешаем UDP-трафик от этого клиента на порт 12345
    iptables -A INPUT -s "$CLIENT_IP" -p udp --dport 12345 -j ACCEPT
done

# Блокируем весь остальной входящий трафик
iptables -A INPUT -j DROP

./server
