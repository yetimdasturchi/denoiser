# denoiser
Simple audio denoiser based on fast furrier method

#compiling

```
make clean
make
```

#using
```bash
./denoise example/noise.wav example/input.wav output.wav 1024 512 2 0.02
aplay output.wav
```
