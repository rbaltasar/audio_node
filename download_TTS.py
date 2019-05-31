# This Python file uses the following encoding: utf-8

print("Loading packages")

from gtts import gTTS
import sys

print("Packages loaded")

if(len(sys.argv) > 2):

    filename = str(sys.argv[1])
    texto = str(sys.argv[2])
    print("Starting TTS download")
    tts = gTTS(text=texto.decode('utf-8'), lang='es')
    print("Saving audio in local memory")
    tts.save(filename)
    #os.system("mpg321 " + filename)
