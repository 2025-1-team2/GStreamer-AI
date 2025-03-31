# 기술 스택
- WSL: 2.4.13.0
- Ubuntu:  22.04.3
- Gemini: gemini-2.0-flash
- GStreamer: 1.26.0

# 실행 방법

```git clone "https://github.com/2025-1-team2/GStreamer-AI.git"```

```cd GStreamer-AI```

```export GEMINI_API_KEY="{{ 발급받은 Gemini API Key }}"```

```gcc -o test0 test0.c -lcurl -ljansson `pkg-config --cflags --libs gstreamer-1.0```

```./test0```
