# esphome-aip1668

ESPHome-Komponente für den LED-Treiber **AiP1668** (Wuxi I-CORE, TM1668-kompatibel) mit Tastenabfrage. Entstanden für die Bedienplatine des SwitchBot Air Purifier (W5302300), funktioniert aber mit jeder Schaltung mit AiP1668 oder TM1668.

Die Komponente ist ein reiner Treiber:

- jede LED (ein Bit im Display-RAM) als `output` (binär), z. B. für `switch: platform: output` oder `light: platform: binary`
- jede Taste als `binary_sensor`
- Helligkeit, Display an/aus und direkter RAM-Zugriff per Lambda

## Einbinden

```yaml
external_components:
  - source: github://EinRainerZufall/esphome-aip1668
    components: [aip1668]

aip1668:
  id: pult
  clk_pin: GPIO22
  dio_pin: GPIO23
  stb_pin: GPIO19
  display_mode: 6x11      # 4x13, 5x12, 6x11, 7x10 (Grids x Segmente)
  brightness: 3           # 0..7
  half_period: 10us       # Takt-Halbperiode, 1..100 us
  update_interval: 20ms   # Tastenabfrage
  refresh_interval: 1s    # alles regelmäßig neu schreiben, 0 = aus

output:
  - platform: aip1668
    id: led_filter
    address: 11           # Byte im Display-RAM (0..13)
    bit: 3                # 0..7

binary_sensor:
  - platform: aip1668
    name: "Taste Ein-Aus"
    key_byte: 0           # Tastenbyte aus Befehl 0x42 (0..4)
    bit: 0
```

Ein vollständiges Beispiel mit allen LEDs und Tasten der SwitchBot-Bedienplatine liegt in [`example/switchbot-bedienpult.yaml`](example/switchbot-bedienpult.yaml).

## Lambda-Funktionen

```cpp
id(pult).set_brightness(5);          // 0..7
id(pult).set_display_on(false);
id(pult).set_bit(0, 7, true);        // Adresse, Bit, Zustand
id(pult).set_byte(5, 0x0B);
id(pult).clear();
uint8_t k = id(pult).get_key_byte(0);
```

## Hinweise

- **DIO wird beim Schreiben push-pull getrieben** und nur nach dem Lesebefehl `0x42` auf Eingang geschaltet. Im Open-Drain-Betrieb mit internen Pull-ups steigt DIO zu langsam (gemessen ca. 13 µs), der Chip liest dann nur Nullen.
- Das Display-RAM wird nur bei Änderungen übertragen (plus `refresh_interval`). Die Tasten werden in jedem `update_interval` gelesen.
- Bei der SwitchBot-Bedienplatine braucht die graue Leitung an J6 High, sonst bleiben alle LEDs dunkel. Sie wird im Beispiel per PWM als Helligkeit genutzt. Die blaue Leitung ist die WLAN-LED (aktiv Low).

## Zuordnung SwitchBot Air Purifier (W5302300)

| LED | address | bit |
|---|---|---|
| Stufe 1 / 2 / 3 | 0 | 7 / 6 / 5 |
| Auto / Pet / Sleep | 2 | 5 / 6 / 7 |
| Timer 2h / 4h / 6h | 5 | 0 / 1 / 3 |
| Kindersicherung | 7 | 0 |
| verdeckte LED | 9 | 1 |
| Filter | 11 | 3 |

| Taste | key_byte | bit |
|---|---|---|
| Ein/Aus | 0 | 0 |
| Stufe | 0 | 3 |
| Modus | 1 | 0 |
| Timer | 1 | 3 |
| Kindersicherung | 2 | 0 |
