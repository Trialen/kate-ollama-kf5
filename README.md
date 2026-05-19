# Kate-Ollama-kf5
[![License](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)   

The Ollama plugin for Kate, refactored to run with the kf5 libraries.

Write a text that starts with `// AI: [your prompt]`.

[(Tiny) Video](https://github.com/user-attachments/assets/a26e3b90-9a32-4092-82be-bf874fbd7c4f)

## Commands

* `Ctrl + /`: prints `// AI: `
* `Ctrl + ;`: execute Ollama with the `generate` endpoint, so doesn't have memory of what was already executed
* `Ctrl + Shift + ;`: execute Ollama with the `generate` endpoint, but with the whole content injected before the prompt

## Configuration

Open the plugin settings page (Settings → Configure Kate → Plugins → Ollama):

* **Ollama URL**: address of your Ollama server (default: `http://localhost:11434`)
* **Available Models**: select the model to use
* **System Prompt**: instructions sent to the model before every request
* **Response Destination**:
  * *Current document* (default): the AI response is inserted at the cursor position in the active document
  * *Named document*: the response is appended to a document with the given name. If a document of that name is already open, the response is appended there. Otherwise, a new in-memory document is created and opened. The default name is `AI Response`.

## Installation instructions

Build and install:

```
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Ninja ../
ninja
sudo ninja install
```

If you are developing, a symlink can simplify loading the latest build without reinstalling:

```
sudo ln -s /your-folder/build/plugins/ktexteditor/kateollama.so /usr/lib/x86_64-linux-gnu/qt5/plugins/kf5/ktexteditor/kateollama.so
```
