### Настройка и отладка геймпада (DualSense / Xbox)

При подключении современных геймпадов (особенно Sony DualSense) по Bluetooth, ядро Linux распознает устройство как несколько виртуальных контроллеров. Драйвер разделяет физические кнопки, сенсорную панель (Touchpad) и гироскоп (Motion Sensors) на разные порты `/dev/input/js*`.

Если в конфигурации запуска ошибочно указать порт гироскопа, узел `joy_node` начнет публиковать данные с частотой 200-300 Гц, перегружая сеть DDS и вызывая ложные срабатывания экстренного торможения (тайм-ауты в `twist_mux`).

#### Как найти правильный порт контроллера

**Шаг 1: Установите утилиты для тестирования джойстиков**
Для Ubuntu/Debian:
```bash
sudo apt install joystick
```
Для Arch Linux:
```bash
sudo pacman -S joyutils
```

**Шаг 2: Посмотрите список доступных устройств**
```bash
ls /dev/input/ | grep js

# Пример вывода:
js0
js1
```

**Шаг 3: Протестируйте устройства по очереди**
Запустите `jstest` для первого порта и подвигайте стиками / понажимайте кнопки:
```bash
jstest /dev/input/js0

# Пример для физических кнопок
Driver version is 2.1.0.
Joystick (DualSense Wireless Controller) has 8 axes (X, Y, Z, Rx, Ry, Rz, Hat0X, Hat0Y)
and 13 buttons (BtnA, BtnB, BtnX, BtnY, BtnTL, BtnTR, BtnTL2, BtnTR2, BtnSelect, BtnStart, BtnMode, BtnThumbL, BtnThumbR).
Testing ... (interrupt to exit)
Axes:  0:  1032  1:  -517  2:-32767  3:   774  4: -1291  5:-32767  6:     0  7:     0 Buttons:  0:off  1:off  2:off  3:off  4:off  5:off  6:off  7:off  8:off  9:off 10:off 11:off 12:off

# Пример для гироскопа (Motion Sensors)
Driver version is 2.1.0.
Joystick (DualSense Wireless Controller Motion Sensors) has 6 axes (X, Y, Z, Rx, Ry, Rz)
and 0 buttons ().
Testing ... (interrupt to exit)
Axes:  0:  -175  1:  8436  2:  1219  3:    -3  4:    -4  5:    -1
```

**Шаг 4: Обновите Launch-файл**
Найдя правильный порт (в примере выше это `js0`), укажите его в `src/mona_core/launch/teleop.launch.py`:
```python
		Node(
            package='joy',
            executable='joy_node',
            name='joy_node',
            parameters=[{
                'dev': '/dev/input/js0',  # Укажите проверенный порт здесь
                'deadzone': 0.05,
                'autorepeat_rate': 20.0,
            }]
        ),
```