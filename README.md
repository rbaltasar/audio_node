# Audio node
## Functionality
This is a virtual node. It does not require a specific hardware (except a speaker to play the audio), and can be executed in the brain of the smart home or any other PC that is connected to the same network as the MQTT broker.

The basic functionality of the node is the following:
* Play an audio file given its absolute path in memory.
* Do real-time Text To Speech (TTS), given input text:
  1. Convert given text to an audio file.
  2. Save the audio file locally into a temporary file.
  3. Play the audio file.
  4. Remove the temporary file.
* Do TTS for later usage, given input text and target audio file in memory (absolute path):
  1. Convert given text to an audio file .
  2. Save the audio file.

Sometimes using always the same sentence for an event is quite boring. Therefore I have extended the basic functionality with the following features:
* A config file (JSON) contains a list of possible reactions to an input event and a target person (e.g: entrance event, person A). These reactions, for the moment, can be either TTS or play some audio file.
* For each event and person, unlimited reactions can be defined.
* An audio file reaction is defined by the tag “music” and the path to the file.
* A TTS reaction is defined by the tag “TTS” and the text to say.
* Under request (Dashboard button), the whole config file will be parsed and all the “TTS” reactions will be generated, downloaded and stored in memory. This step is needed each time the list of reactions is modified, as it uses the “do TTS for later usage” functionality.
* Every time the node is called with an event E and a target person A, a random reaction from the list  for that event and person will be chosen and played.

So basically this node allows you to output audio, whenever you want, saying whatever you want and under the automation conditions you define.
If you decide to play music, just give the path to the music file. If you decide to convert text to speech, either give the text to say or use the offline functionality to save your text for later.
If you want that an event triggers some audio/voice, but you don’t want to reuse always the same song/sentence, just reuse the JSON config concept (e.g: Under entrance event: “Hello” or “Welcome” or “Welcome back!”...).

## Implementation
The easiest solution I could find for TTS is gTTS, which has a very easy-to-use python library. It is compatible with many languages and the voice quality is good enough (not as good as Alexa, for example, but it makes its job).
I created a small python script that gets as input the text to translate to voice and the path and filename to store the result in memory. The script internally will use the gTTS API to download the audio file and save it into the given memory location under the given name.

This node will be used in Node-Red at many places, wherever you want to have some audio interaction triggered by an event. Different events may be fired very close in time and they may call this node. This is a problem because we don’t want to hear two audio files at the same time.

We clearly have a shared resource (speaker) that must only be used once at a time.
We are not in the classical mutex context, as the resource is not shared within threads but within processes, we need to use an interprocess synchronization mechanism.
Unfortunately Python does not support such a mechanism (or at least I couldn’t find any documentation about it). For this we need to use the Boost Interprocess library in C++.
I couldn’t find a C++ implementation of the gTTS library, so I decided to create a C++ wrapper that takes care of the synchronization mechanisms and high-level logic, and that calls the python script, that takes care of the TTS functionality.

Another thing we need, is an argument parser and a JSON parser, to simplify giving input arguments to the program, and reading the config file. These two things are plug & play libraries in Python, but unfortunately we need this at the C++ wrapper level, so we need to use additional libraries for this (see external libraries used) that are not so easy to integrate.

This node is also time-critical, as we want to hear the audio reactions right when the event happens. To make the execution as fast as possible in case of parallelism, we the interprocess mutex only protects the speaker resource, all the other tasks (parse config file, download, store in memory…) can happen in parallel.

In case of real-time translation (audio file stored in a temporary file and removed after played) we need to be careful to use unique names for the temporary file. Otherwise two processes may try to save their audio file under the same name. To avoid this, I will just name each temporary file after the process handling it (PID). This way we can be sure that all temporary objects stored in memory (and later-on removed) have unique names.

## Known issues
* The performance of the real-time TTS is very resource/internet dependent, and therefore, the response time may vary between a few seconds an a few minutes (yes, once it took up to 5 minutes do the complete process!). Therefore for sentences that do not change (e.g: “See you later!”) it is recommended to use the offline functionality (download once the audio file and reuse it many times). In this case the latency is almost zero. I would recommend to only use the real-time TTS for sentences whose content changes dynamically (e.g: “The temperature of the attic is 20°”).

## Future improvements
* Improve the configuration concept. Make it easier for the user to add/remove entries.
## Used hardware
* Any speaker

## External libraries used
* Audio player: mpg321
* TTS: https://pypi.org/project/gTTS/
* Boost interprocess: https://www.boost.org/doc/libs/1_63_0/doc/html/interprocess.html
* Boost program options: https://www.boost.org/doc/libs/1_58_0/doc/html/program_options.html
* C++ JSON Parser: https://github.com/nlohmann/json
