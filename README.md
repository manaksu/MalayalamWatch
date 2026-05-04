# Keralam — Malayalam Watchface

Pebble Time Steel watchface with Malayalam numerals on a cream e-paper background.

## Features
- 12 Malayalam numeral bitmaps (൧–൧൨) around the screen perimeter
- Analog hands — thick hour, thin minute
- Cream background (`#f0ece0`) with soft black ink
- Full 144×168 screen, clock center at (72, 84)

## Import into CloudPebble

1. Go to [cloudpebble.net](https://cloudpebble.net)
2. **Import** → **Import from GitHub**
3. Paste your repo URL
4. Build & run

## Project structure

```
keralam/
├── appinfo.json
├── src/
│   └── main.c
├── resources/
│   └── images/
│       ├── num_01.png  ൧
│       ├── num_02.png  ൨
│       ├── ...
│       └── num_12.png  ൧൨
└── README.md
```

## Roadmap
- [ ] Chundan Vallam battery indicator (lower half)
- [ ] Date display
