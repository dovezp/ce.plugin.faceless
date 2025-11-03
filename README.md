# Faceless - The Cheat Engine Anti-Capture Plugin

## About

Faceless is the next evolution in my Cheat Engine plugin suite, building on the success of [Nameless Plugin](https://github.com/dovezp/ce.plugin.nameless). This plugin is designed to protect Cheat Engine by making its interface invisible to windows screen capture techniques, including screenshots, screen recordings, and screen capture utilities.

## Building

### Building Environment

* Visual Studio 2022 with Windows SDK 10 & 11 for System Support of Windows 10 - 11

### System Requirement

* Recommended Minimum Microsoft Windows 10 Enterprise LTSC (Version	10.0.19044 Build 19044)
* Cheat Engine 7.5 / Cheat Engine 7.6

### Installation

#### Plugin Reminder

* The x86 `faceless` build is compatible with `cheatengine-i386.exe`
* The x64 `faceless` build is compatible with `cheatengine-x86_64.exe` and `cheatengine-x86_64-SSE4-AVX2.exe`
* Both x86 and x64 `faceless` builds reference the same `faceless.ini` file

#### Plugin Setup

1. Extract the most recent [faceless](https://github.com/dovezp/ce.plugin.faceless/releases) build into the root directory for `Cheat Engine`
    * The root directory where `cheatengine-i386.exe` / `cheatengine-x86_64.exe` is located
2. If needed, configure the `faceless.ini` settings file
3. Start Cheat Engine
4. Go to Edit -> Settings -> Plugins -> Add new
5. Add the faceless dll associated with the Cheat Engine executable (x64 or x86)
6. Click the checkbox next to the faceless dll plugin name to enable 
7. Click Okay to continue
8. At this point the plugin should be running. 
9. Give it a test and try to capture with the Snipping Tool.
10. If you encounter an issue try restarting Cheat Engine or build with Debug mode and look at the output logs with dbgview. 

### Configuration File Settings

The configuration file contains various settings that control the behavior of the application.

* `Enabled` (Boolean Value)
  * Set this to `True` if you want the plugin to hide from screen capture. If set to `False`, the plugin will operate in a disabled state. In most cases you will just enable / disable the plugin via the Cheat Engine Plugin Setting Window.

#### Default Configuration File Settings

Here is an example of the default configuration in `faceless.ini`:

```
[Settings]
Enabled = true
```
Feel free to modify these settings in the configuration file according to your requirements.


## License

This project operates under the [Apache License 2.0 (Apache-2.0)](https://tldrlegal.com/license/apache-license-2.0-(apache-2.0)). Please refer to the [LICENSE.md](./LICENSE.md) file for detailed information.

## Your Feedback Counts

Your insights and feedback, whether positive or constructive, are immensely valuable. Your contributions guide the refinement of this plugin for future iterations.

Share your thoughts by opening an issue in the [repository's issue section](https://github.com/dovezp/ce.plugin.faceless/issues). Be sure to provide context and links when sharing your feedback.

Thank you for being an essential part of this plugin's growth journey.

### Anti-AI Notice

I do not support or condone the use of artificial intelligence (AI) tools in the creation or distribution of any content I produce. All work published by me is made by a human, and I will not grant permission for my content, artwork, writing, or any related materials to be used in training or generating AI models. 

I request that no one uploads, scrapes, or utilizes my work in any AI datasets or projects. Any violation of this request goes against my explicit wishes.

Thank you for respecting human creativity.

---

<p align="center">
  <p align="center">
    <a href="https://github.com/dovezp/ce.plugin.faceless/releases">
      <img src="https://img.shields.io/github/downloads/dovezp/ce.plugin.faceless/total?style=flat-square" alt="downloads"/>
    </a>
    <a href="https://github.com/dovezp/ce.plugin.faceless/graphs/contributors">
      <img src="https://img.shields.io/github/contributors/dovezp/ce.plugin.faceless?style=flat-square" alt="contributors"/>
    </a>
    <a href="https://github.com/dovezp/ce.plugin.faceless/watchers">
      <img src="https://img.shields.io/github/watchers/dovezp/ce.plugin.faceless?style=flat-square" alt="watchers"/>
    </a>
    <a href="https://github.com/dovezp/ce.plugin.faceless/stargazers">
      <img src="https://img.shields.io/github/stars/dovezp/ce.plugin.faceless?style=flat-square" alt="stars"/>
    </a>
    <a href="https://github.com/dovezp/ce.plugin.faceless/network/members">
      <img src="https://img.shields.io/github/forks/dovezp/ce.plugin.faceless?style=flat-square" alt="forks"/>
    </a>
  </p>
</p>

<p align="center">
  <a href="https://github.com/dovezp">
    <img width="64" heigth="64" src="https://avatars.githubusercontent.com/u/89095890" alt="dovezp"/>
  </a>  
</p>
