#!/bin/bash
echo "Kompilowanie projektu..."
make main
if [ $? -ne 0 ]; then
    echo "Błąd kompilacji!"
    exit 1
fi
echo "Uruchamianie serwera GUI..."
python3 server.py
