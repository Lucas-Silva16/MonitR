# Monitr

Smart environment and security monitoring system that combines an ESP32 sensor node, a Python analytics service, and a PHP web dashboard to detect anomalies in real time — such as unusual temperature/humidity changes, unexpected motion, or abnormal network bandwidth usage.

I am currently expanding an existing ESP32 room monitoring project by developing a new Python analytics layer.
 
This initiative is heavily inspired by my current summer internship in the Infrastructure and Communications department at Dr. Nélio Mendonça Hospital. In my day-to-day role, I am developing practical skills in system administration and networking by visiting multiple healthcare centers, where I manage infrastructure and ensure seamless network integration for essential medical hardware.



##  Goal

Turn simple room sensor data into actionable alerts, providing a low-cost monitoring solution for spaces like server rooms, storage areas, small offices, or home environments.

##  Architecture

```
ESP32 (sensors) → PHP Backend + MySQL (storage) → Python Service (analysis/alerts) → PHP Dashboard (visualization)
```

- **ESP32** — reads temperature, humidity and bandwidth data
- **PHP + MySQL** — receives and stores sensor data, serves the web dashboard
- **Python** — reads stored data, detects anomalies, and triggers alerts

##  Tech Stack

- C++ (ESP32 firmware)
- PHP + MySQL (backend & storage)
- Python (data analysis / anomaly detection — pandas, scikit-learn)
- JavaScript / Chart.js (dashboard visualization)
