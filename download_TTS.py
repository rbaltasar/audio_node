# This Python file uses the following encoding: utf-8
from gtts import gTTS
import sys


if(len(sys.argv) > 2):

    filename = str(sys.argv[1])
    texto = str(sys.argv[2])
    tts = gTTS(text=texto.decode('utf-8'), lang='es')
    tts.save(filename)
    #os.system("mpg321 " + filename)
