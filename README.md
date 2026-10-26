# Cab Booking Application (C++)

Console-based cab booking system built for **Sqrock IT Solutions - Internship Project Phase 1, Task 1**.
Passengers register, book Mini / Sedan / SUV cabs, see fare estimates, track trips, pay and rate drivers.
Drivers accept or reject rides and update the trip status. An admin panel shows users, drivers, trips and revenue.

## Build and run

Requires a C++17 compiler (g++ / clang++ / MinGW).

```bash
make run          # builds and starts the app
# or manually
g++ -std=c++17 -O2 -o cab main.cpp
./cab
```

Data is created automatically in `data/` (CSV files) and `receipts/` on first run. Delete those folders to reset.

### Demo logins

| Role | Login | Password |
|------|-------|----------|
| Passenger | register yourself (phone number is the ID) | your choice |
| Driver | `D101` ... `D106` | `driver123` |
| Admin | `admin` | `admin123` |

Drivers: D101-D102 Mini, D103-D104 Sedan, D105-D106 SUV.

## Features

**Required**
- Register, login, logout (name, phone, password; last pickup and destination are remembered)
- Book a cab: choose pickup/drop from a list of places, pick Mini / Sedan / SUV, see the estimated fare for all three
- Fare = base fare + distance x per-km rate, optional **night charge** (+25% for pickups 22:00-05:00) and **waiting charge** (after 5 free minutes)
- View current booking, cancel a booking (before the trip starts), trip history
- **Driver module**: accept / reject rides, view ride details, update status Booked -> Ongoing -> Completed (or Cancelled)

**Bonus (all implemented)**
- GPS / location simulation: every place has coordinates, distance uses the haversine formula x 1.25 road factor
- Driver allocation logic: nearest available driver of the chosen cab type; if a driver rejects, the ride is re-allocated to the next nearest one
- Payment simulation (Cash / UPI / Card) with transaction IDs
- Ride rating system (1-5), driver average rating
- Booking receipt generation (printed and saved to `receipts/RCPT_<tripId>.txt`)
- Admin panel: users, drivers, all trips, revenue report (`report.txt`)

## Fare table

| Cab | Base | Per km | Waiting / min | Seats |
|-----|------|--------|---------------|-------|
| Mini | Rs. 40 | Rs. 10 | Rs. 1.50 | 4 |
| Sedan | Rs. 60 | Rs. 13 | Rs. 2.00 | 4 |
| SUV | Rs. 90 | Rs. 18 | Rs. 3.00 | 6 |

## Project structure

| File | Module |
|------|--------|
| `main.cpp` | Menus and program flow |
| `utils.h` | Input validation, string / time helpers, password hashing |
| `models.h` | Entities: `User`, `Driver`, `Trip`, `CabType`, `CityMap` (location data + distance) |
| `fare.h` | **Fare Calculation Module** (`FareCalculator`) |
| `auth.h` | **Authentication Module** (register / login for users, drivers, admin) |
| `booking.h` | **Booking Module**, `Dispatcher` (driver allocation), trip history, payment, receipts |
| `driver_module.h` | **Driver Module** |
| `admin.h` | **Admin panel** |
| `database.h` | **File Management Module** (CSV read / write) |

## C++ concepts used

- **OOP**: classes with static utility methods (`FareCalculator`, `Dispatcher`, `CityMap`), data classes (`User`, `Driver`, `Trip`), one class per module, separation of data, logic and UI
- **STL**: `vector`, `map`, `string`, `stringstream`, `sort`, `find`, lambdas, `iomanip` formatting
- **File handling**: `ifstream` / `ofstream` CSV persistence for users, drivers and trips, receipt text files
- **Data structures and algorithms**: nearest-driver search, haversine distance, record lookup
- **Validation / robustness**: phone number, time, menu ranges, duplicate registration, one active booking per user, graceful exit when input ends

## Data files

`data/users.csv`, `data/drivers.csv`, `data/trips.csv`, `receipts/*.txt`, `report.txt`.
Passwords are stored as salted hashes (`std::hash`) - suitable for a learning project, not for production security.

## Sample run

`sample_output.txt` contains the console output of a full run: register -> book (night fare) -> driver accepts, starts, completes with waiting time -> passenger pays by UPI, rates the driver and gets a receipt.

## Possible improvements

Real map / GPS API, SQLite database, surge pricing, ride sharing, GUI.
