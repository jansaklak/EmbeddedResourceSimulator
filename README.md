# Embedded Resource Task Simulator

Zaawansowany symulator C++17 oraz nowoczesny interfejs graficzny Web GUI przeznaczony do symulacji, wizualizacji i optymalizacji szeregowania zadań w systemach wbudowanych o ograniczonych zasobach sprzętowych.

---

## 🚀 Szybkie Uruchomienie

### 1. Interfejs Graficzny (Web GUI)
Interaktywny interfejs z **symulacją w czasie rzeczywistym (Live Simulation)**, regulacją tempa odtwarzania, dynamicznymi kartami zasobów sprzętowych, wykresami Gantta, wizualizacją grafu (DAG) oraz porównywarką strategii:

```bash
make gui
```
Lub bezpośrednio skryptem:
```bash
./run_gui.sh
```
Aplikacja otworzy się automatycznie w przeglądarce pod adresem: `http://localhost:5050`

---

### 2. Tryb Konsolowy (CLI)
Tradycyjny interfejs wiersza poleceń w języku C++:

```bash
make clean && make run
```

---

### 3. Eksport JSON w Trybie Wsadowym
Możliwość wywoływania silnika C++ z wiersza poleceń do eksportu wyników w formacie JSON:

```bash
# Eksport dla konkretnego pliku i strategii S8 (Optymalizacja Kosztowo-Czasowa)
./main --export-json data/graph20.dat 8

# Eksport dla losowego grafu 20 zadań
./main --export-json-rand 20 4 4 1 1 0 8
```

---

## 💡 Główne Funkcjonalności

- **⚡ Symulacja Live w Czasie Rzeczywistym**:
  - Kontrola odtwarzania (`▶ Uruchom`, `⏸ Pauza`, `⏭ Krok`, `🔄 Reset`).
  - Suwak tempa odtwarzania (`20 ms` - `1000 ms`/tyknięcie) i szybkie skróty prędkości (`0.5x`, `1x`, `2x`, `5x`, `10x`).
  - Karty zasobów procesorów (`🟢 AKTYWNY` / `⚪ BEZCZYNNY`) z czasem wykonania i paskiem postępu %.
  - Podgląd kolejki zadań w czasie rzeczywistym (**🔵 W trakcie**, **⏳ Oczekujące**, **✅ Zakończone**).
  - Dynamiczne przełączanie strategii szeregowania w locie.

- **🏗️ Modułowa Architektura C++17**:
  - `src/models/`: Czystszy podział klas domeny (`TaskGraph`, `HardwareProcessor`, `HardwareInstance`, `ExecutionMatrix`, `CommunicationBus`, `SubTaskManager`, `Edge`, `ConfigParser`).
  - `src/simulator/`: Dedykowane moduły silnika symulacyjnego `TaskSchedulerSimulator`.

- **📊 Wielordzeniowe Benchmarki**:
  - Serwer API Python wywołuje równolegle silnik symulacji na wszystkich dostępnych rdzeniach procesora.

---

## 📚 Dokładna Dokumentacja Techniczna

Szczegółowy opis architektury C++17, algorytmów szeregowania, podbudowy matematycznej, rafinacji z funkcją kary, specyfikacji formatu `.dat` oraz interfejsu API znajduje się w pliku:

➡️ **[DOCUMENTATION.md](DOCUMENTATION.md)**

---

## 🛠️ Wymagania

- Kompilator C++ wspierający C++17 (`g++` lub `clang++`)
- Python 3.x (do uruchomienia serwera GUI `server.py`)
- Narzędzie `make`
