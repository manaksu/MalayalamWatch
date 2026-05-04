# Keralam — Malayalam Watchface

A Pebble Time Steel watchface celebrating Kerala.

## Features

**Clock (top 144×84)**
- Malayalam numerals (൧–൧൨) around the perimeter
- 60 tick marks (major at each hour, minor every minute)
- Analog pixel hands — thick hour, thin minute
- Cream e-paper background (`#f0ece0`)
- Archimedean spiral battery indicator — inside clock face, left side

**Date strip (below clock)**
- Line 1: `04 മേയ്, 26` — Arabic day + Malayalam month + Arabic YY
- Line 2: `തിങ്കൾ` — Day of week in Malayalam

**Bottom inscription**
- `കേരളം, നവംബർ 1, 1956` — Kerala formation date, centered

## Resources (32 total)
| Resource | Count | Description |
|---|---|---|
| `num_01`–`num_12` | 12 | Clock hour numerals |
| `month_01`–`month_12` | 12 | Malayalam month names |
| `day_0`–`day_6` | 7 | Malayalam weekday names |
| `inscription` | 1 | Kerala formation date |

## Import into CloudPebble

1. Push to GitHub
2. CloudPebble → **Import** → **Import from GitHub**
3. Paste repo URL → Build

## Structure

```
keralam/
├── appinfo.json
├── src/
│   └── main.c
├── resources/
│   └── images/
│       ├── num_01.png … num_12.png
│       ├── month_01.png … month_12.png
│       ├── day_0.png … day_6.png
│       └── inscription.png
└── README.md
```

## Roadmap
- [ ] Chundan Vallam in lower half
