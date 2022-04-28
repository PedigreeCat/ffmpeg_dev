目录结构

```
├─include
│  ├─fdk-aac        
│  ├─lame
│  ├─libavcodec     
│  ├─libavdevice    
│  ├─libavfilter    
│  ├─libavformat    
│  ├─libavutil      
│  ├─libpostproc    
│  ├─libswresample  
│  ├─libswscale     
│  ├─SDL2
│  └─speex
├─lib
│  ├─cmake
│  │  └─SDL2        
│  └─pkgconfig  
└─src
    ├─libavcodec
    │  ├─aarch64
    │  ├─alpha  
    │  ├─arm    
    │  ├─avr32  
    │  ├─bfin
    │  ├─mips
    │  ├─neon
    │  ├─ppc
    │  ├─sh4
    │  ├─sparc
    │  ├─tests
    │  │  ├─aarch64
    │  │  ├─arm
    │  │  ├─ppc
    │  │  └─x86
    │  └─x86
    ├─libavdevice
    │  └─tests
    ├─libavfilter
    │  ├─aarch64
    │  ├─dnn
    │  ├─opencl
    │  ├─tests
    │  └─x86
    ├─libavformat
    │  └─tests
    ├─libavresample
    │  ├─aarch64
    │  ├─arm
    │  ├─tests
    │  └─x86
    ├─libavutil
    │  ├─aarch64
    │  ├─arm
    │  ├─avr32
    │  ├─bfin
    │  ├─mips
    │  ├─ppc
    │  ├─sh4
    │  ├─tests
    │  ├─tomi
    │  └─x86
    ├─libpostproc
    ├─libswresample
    │  ├─aarch64
    │  ├─arm
    │  ├─tests
    │  └─x86
    └─libswscale
        ├─aarch64
        ├─arm
        ├─ppc
        ├─tests
        └─x86
```

src目录下为实际编写的代码。