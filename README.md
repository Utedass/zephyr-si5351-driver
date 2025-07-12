# Si5351 Driver for Zephyr

This readme currently serves as a working document for the driver, to figure out what I want to implement and how. It does this by notes and proposed documentation.

Things not planned for implementation right now:
- VCXO support
- Spread Spectrum support

## Device tree entry

```c
&i2c1 {
    cg1: si5351@60 {
        compatible = "skyworks,si5351";
        reg = <0x60>;               // I2C address
        #address-cells = <1>;       // Child-node setting, set to <1>
        #size-cells = <0>;          // Child-node setting, set to <0>

        model = "si5351a-b-gt";     // Possible values see below

        xtal-frequency = <25>;      // Crystal frequency in MHz, 25 or 27
        xtal-load = <10>;           // Crystal load capacitance in pf, 6, 8 or 10

        clkin-freq = <10000000>;    // CLKIN frequency in Hz
        clkin-div = <1>;            // 1, 2, 4, 8

        fvco-max = <900>;           // In MHz, allow override of 900 MHz spec
        fvco-min = <400>;           // In MHz, allow override of 600 MHz spec

        plla-clock-source = "CLKin";
        plla-fixed-multiplier;              // Locks the plla multiplier ratio
        plla-frequency = <420000000>;       // Either set the desired frequency in Hz
        plla-frequency-fractional = <0>;    // and optional fractional parts
        
        plla-a = <16>;                      // XOR set the multiplier manually
        plla-b = <800>;                     // in the form a + b / c
        plla-c = <1000>;

        plla-p1 = <1638>;                   // XOR set the parameters manually
        plla-p2 = <400>;
        plla-p3 = <1000>;

                                            // Same for PLLb
        pllb-clock-source = "XTAL";
        pllb-p1 = <1337>;
        pllb-p2 = <7>;
        pllb-p3 = <29>;

        // Define the clock outputs. No requirements on the name.
        // This node is compatible with the Clock Control API
        // Supported API calls are _on, _off, 
        // To come: _async_on, _get_status, _get_rate
        // Runtime configuration only available through si5351 API
        clkout0: clock@0 {
            compatible = "skyworks,si5351-output";  // Enforce binding schema
            reg = <0>;                              // Denotes which output index this configuration applies to
            #clock-cells = <0>;                     // Must be 0
            output-enabled;                         // Enable output at boot
            powered-up;                             // Power up output at boot    
            clock-source = "multisynth";            // Clock source, "xtal", "clkin" or "multisynth"
            multisynth-source = "PLLA";             // Multisynth source, "PLLA" or "PLLB"
            drive-strength = <8>;                   // Output drivestrength in mA, 2, 4, 6 or 8
            fixed-divider;                          // Whether to lock this divider

            frequency = <3500000>;                  // Set output frequency in Hz
            frequency-fractional = <15>             // Output frequency fractional part

            plla-a = <16>;                          // XOR set the multiplier manually
            plla-b = <800>;                         // in the form a + b / c
            plla-c = <1000>;

            p1 = <14848>;                           // XOR set the parameters manually
            p2 = <0>;                               // 
            p3 = <1>;                               // 
            r = <1>;                                // output divider 1, 2, 4, 8, 16, 32, 64 or 128

            phase-offset = <0>;                     // Set initial phase offset
        };

        clkout1: clock@1 {
            compatible = "skyworks,si5351-output";
            reg = <1>;
            ...
        };

        clkout2: clock@2 {
            compatible = "skyworks,si5351-output";
            reg = <2>;
            ...
        };
    };
};
```

| Model          | PLLs | CLKIN | VCXO | Outputs | Package  |
|----------------|------|--------|------|---------|----------|
| si5351a-b-gt   | 2    | No     | No   | 3       | 10-MSOP  |
| si5351a-b-gm1  | 2    | No     | No   | 4       | 16-QFN   |
| si5351a-b-gm   | 2    | No     | No   | 8       | 20-QFN   |
| si5351b-b-gm1  | 1    | No     | Yes  | 4       | 16-QFN   |
| si5351b-b-gm   | 1    | No     | Yes  | 8       | 20-QFN   |
| si5351c-b-gm1  | 2    | Yes    | No   | 4       | 16-QFN   |
| si5351c-b-gm   | 2    | Yes    | No   | 8       | 20-QFN   |


## si5351 API

### Functions

```c

si5351_reconfigure(const struct device *dev);
si5351_load_dt_settings(const struct device *dev);

si5351_set_clkin(const struct device *dev, uint32_t frequency, si5351_clkin_div_t div);

si5351_is_xtal_running(const struct device *dev)

si5351_pll_soft_reset(const struct device *dev, uint8_t pll_index);
si5351_pll_set_frequency(const struct device *dev, uint8_t pll_index, float frequency);
si5351_pll_set_multiplier(const struct device *dev, uint8_t pll_index, float multiplier);
si5351_pll_set_multiplier_abc(const struct device *dev, uint8_t pll_index, uint32_t a, uint32_t b, uint32_t c);
si5351_pll_set_multiplier_parameters(const struct device *dev, uint8_t pll_index, uint32_t p1, uint32_t p2, uint32_t p3);
si5351_pll_set_divider_fixed(const struct device *dev, uint8_t pll_index, bool is_fixed);

si5351_pll_get_frequency(const struct device *dev, uint8_t pll_index, float *frequency);
si5351_pll_get_multiplier(const struct device *dev, uint8_t pll_index, float *multiplier);

si5351_pll_is_fixed(const struct device *dev, uint8_t pll_index);
si5351_pll_is_locked(const struct device *dev, uint8_t pll_index);

si5351_output_set_source(const struct device *dev, uint8_t output_index, si5351_output_source_t source);
si5351_output_set_multisynth_source(const struct device *dev, uint8_t output_index, si5351_output_multisynth_source_t source);
si5351_output_set_frequency(const struct device *dev, uint8_t output_index, float frequency);
si5351_output_set_divider(const struct device *dev, uint8_t output_index, float multiplier);
si5351_output_set_divider_abc(const struct device *dev, uint8_t output_index, uint32_t a, uint32_t b, uint32_t c);
si5351_output_set_divider_parameters(const struct device *dev, uint8_t output_index, uint32_t p1, uint32_t p2, uint32_t p3);
si5351_output_set_divider_integer(const struct device *dev, uint8_t output_index, uint8_t integer);
si5351_output_set_divider_fixed(const struct device *dev, uint8_t output_index, bool is_fixed);
si5351_output_set_powered_down(const struct device *dev, uint8_t output_index, bool is_powered_done);
si5351_output_set_output_enabled(const struct device *dev, uint8_t output_index, bool is_enabled);
si5351_output_set_output_enable_mask(const struct device *dev, uint8_t output_index, bool is_masked);
si5351_output_set_inverted(const struct device *dev, uint8_t output_index, bool is_inverted);
si5351_output_set_phase_offset(const struct device *dev, uint8_t output_index, uint16_t micro_seconds);
si5351_output_set_phase_offset_val(const struct device *dev, uint8_t output_index, uint8_t val);

si5351_output_get_frequency(const struct device *dev, uint8_t output_index, float *frequency);
si5351_output_get_divider(const struct device *dev, uint8_t output_index, float *multiplier);

```

### Types


```c

```

### Constants / Defines

```c

```

