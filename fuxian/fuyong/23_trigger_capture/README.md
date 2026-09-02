# 23_trigger_capture

在一帧 ADC code 中按迟滞上升沿触发，并把触发前 `PRETRIGGER_COUNT` 点和后续点提取到 `captured_samples[]`。

`TRIGGER_CAPTURE` 的输入是 `adc_samples[]`、触发阈值/迟滞（ADC code）；输出是 `captured_samples[]` 和 `trigger_index`。预触发、后触发和缓存都由 `signal_trigger_capture` 现有模块完成；多槽回放继续使用 `signal_single_capture_replay`，不在本工程重复实现。
