# Models

Model files are not committed to this repository.

Place local experiment files here when needed:

- `.onnx`
- TensorRT `.engine`
- calibration data
- model-specific notes

For an ONNX Runtime smoke test, generate a local identity model:

```powershell
py -m pip install -r ..\tools\requirements-model.txt
py ..\tools\create_identity_onnx.py --output identity_audio_1x1x128.onnx `
  --channels 1 --frames 128
```
