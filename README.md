# Si5351 Driver for Zephyr

This readme currently serves as a working document for the driver, to figure out what I want to implement and how. It does this by notes and proposed documentation.

Things not planned for implementation right now:
- VCXO support
- Spread Spectrum support
- Interrupt support
- Thread safety

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

        plla-clock-source = "CLKIN";
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

            clock-source = "multisynth";            // Clock source, "XTAL", "CLKIN" or "multisynth"
            multisynth-source = "PLLA";             // Multisynth source, "PLLA" or "PLLB"

            drive-strength = <8>;                   // Output drivestrength in mA, 2, 4, 6 or 8
            invert;                                 // Invert the output

            fixed-divider;                          // Whether to lock this divider

            frequency = <3500000>;                  // Set output frequency in Hz
            frequency-fractional = <15>             // Output frequency fractional part

            a = <16>;                               // XOR set the multiplier manually
            b = <800>;                              // in the form a + b / c
            c = <1000>;

            p1 = <14848>;                           // XOR set the parameters manually
            p2 = <0>;                               // 
            p3 = <1>;                               // 
            r = <1>;                                // output divider 1, 2, 4, 8, 16, 32, 64 or 128

            phase-offset = <0>;                     // Set initial phase offset
            phase-offset-ps = <0>;                  // XOR set desired initial phase offset in ps
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

int si5351_reapply_configuration(const struct device *dev);
int si5351_apply_dt_settings(const struct device *dev);

int si5351_set_clkin(const struct device *dev, uint32_t frequency, si5351_clkin_div_t div);

int si5351_is_xtal_running(const struct device *dev);

int si5351_pll_soft_reset(const struct device *dev, si5351_pll_mask_t pll_mask);
int si5351_pll_set_frequency(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_source_t source, si5351_frequency_t frequency);
int si5351_pll_set_multiplier(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_ratio_t multiplier);
int si5351_pll_set_multiplier_abc(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t a, uint32_t b, uint32_t c);
int si5351_pll_set_multiplier_parameters(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t p1, uint32_t p2, uint32_t p3);
int si5351_pll_set_multiplier_integer(const struct device *dev, si5351_pll_mask_t pll_mask, uint8_t multiplier);
int si5351_pll_set_divider_fixed(const struct device *dev, si5351_pll_mask_t pll_mask, bool is_fixed);

int si5351_pll_get_frequency(const struct device *dev, si5351_pll_index_t pll_index, si5351_frequency_t *frequency);
int si5351_pll_get_multiplier(const struct device *dev, si5351_pll_index_t pll_index, si5351_ratio_t *multiplier);

int si5351_pll_is_fixed(const struct device *dev, si5351_pll_index_t pll_index);
int si5351_pll_is_locked(const struct device *dev, si5351_pll_index_t pll_index);

int si5351_output_set_source(const struct device *dev, si5351_output_mask_t output_mask, si5351_output_source_t source);
int si5351_output_set_multisynth_source(const struct device *dev, si5351_output_mask_t output_mask, si5351_output_multisynth_source_t source);
int si5351_output_set_frequency(const struct device *dev, si5351_output_mask_t output_mask, si5351_frequency_t frequency);
int si5351_output_set_divider(const struct device *dev, si5351_output_mask_t output_mask, si5351_ratio_t divider);
int si5351_output_set_divider_abc(const struct device *dev, si5351_output_mask_t output_mask, uint32_t a, uint32_t b, uint32_t c);
int si5351_output_set_divider_parameters(const struct device *dev, si5351_output_mask_t output_mask, uint32_t p1, uint32_t p2, uint32_t p3);
int si5351_output_set_divider_integer(const struct device *dev, si5351_output_mask_t output_mask, uint8_t integer);
int si5351_output_set_divider_fixed(const struct device *dev, si5351_output_mask_t output_mask, bool is_fixed);
int si5351_output_set_powered_down(const struct device *dev, si5351_output_mask_t output_mask, bool is_powered_down);
int si5351_output_set_output_enabled(const struct device *dev, si5351_output_mask_t output_mask, bool is_enabled);
int si5351_output_set_output_enable_mask(const struct device *dev, si5351_output_mask_t output_mask, bool is_masked);
int si5351_output_set_inverted(const struct device *dev, si5351_output_mask_t output_mask, bool is_inverted);
int si5351_output_set_phase_offset_ps(const struct device *dev, si5351_output_mask_t output_mask, uint32_t pico_seconds);
int si5351_output_set_phase_offset_val(const struct device *dev, si5351_output_mask_t output_mask, uint8_t val);

int si5351_output_get_frequency(const struct device *dev, si5351_output_index_t output_index, si5351_frequency_t *frequency);
int si5351_output_get_divider(const struct device *dev, si5351_output_index_t output_index, si5351_ratio_t *divider);

```

### Hidden / Static functions

```c

```

### Types


```c
si5351_pll_mask_t
si5351_pll_source_t
si5351_output_mask_t
si5351_output_source_t
si5351_output_multisynth_source_t
```

### Constants / Defines

```c

```

