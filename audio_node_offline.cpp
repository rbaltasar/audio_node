#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/program_options.hpp>
#include "json/single_include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <thread>
#include <string>
#include <sys/types.h>
#include <unistd.h>

using namespace boost::program_options;
using json = nlohmann::json;

#define ABSOLUTE_AUDIO_PATH "/home/raul/Projects/audio_node/sounds"

//Reproduce audio using mpg321 player
//The execution is process-safe: only one process at a time can access the audio resource
//preventing audio overlapping and audio files corruption
void play_audio(std::string audio_file, boost::interprocess::named_mutex& mutex)
{

  try
  {
    //Lock named mutex
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

    //Play the audio file
    std::cout << "Playing: " << audio_file << std::endl;
    std::string command = "mpg321 " + audio_file;
    system(command.c_str());
  }

  //Catch an interprocess exception to avoid permanent locking of the named mutex
  catch(boost::interprocess::interprocess_exception &ex){
      std::cout << ex.what() << std::endl;
      return;
   }
}

//Generate a filename based on the PID of the calling process
//Concurrent process will generate different filenames, avoiding write/read accesses
//to the same resource during file download
std::string generate_tmp_filename()
{
  std::string tmp_filename = std::to_string((int)getpid());
  tmp_filename += ".mp3";
  std::cout << "Generated temporary filename: " << tmp_filename << std::endl;

  return tmp_filename;
}

//Concatenate filename and text to generate a cmd command to call the TTS python
//script, with the following structure: <filename> "<text>""
std::string generate_command(std::string filename, std::string text)
{
  std::string command = "python download_TTS.py ";
  command += filename;
  command += " \"";
  command += text;
  command += "\"";

  std::cout << "Generated command: " << command << std::endl;

  return command;
}

//Generate random index to the JSON dictionary based on the variable
//"amount_of_sentences" provided also in the JSON dictionary
std::string generate_random_index(json& j, std::string event, std::string person)
{
  //Initialize random seed
  srand((int)time(0));

  //Get number of entries in list
  auto num_entries = j[event.c_str()][person.c_str()]["amount_of_sentences"];

  //Generate random index
  auto idx = std::to_string(rand() % (uint8_t)num_entries);

  return idx;
}

std::string generate_persistant_filename(std::string event, std::string person, std::string idx)
{
  std::string filename = ABSOLUTE_AUDIO_PATH;
  filename += "/";
  filename += event;
  filename += "_";
  filename += person;
  filename += "_";
  filename += idx;
  filename += ".mp3";

  std::cout << "Generated filename: " << filename << std::endl;

  return filename;
}

void remove_and_download(json& j,std::string event, std::string person)
{
  //Get amount of entries
  auto num_entries = j[event.c_str()][person.c_str()]["amount_of_sentences"];

  //Loop over all the entries
  for(uint8_t i=0; i < num_entries; i++)
  {
    //Get the entry text
    auto idx = std::to_string(i);
    auto text = j[event.c_str()][person.c_str()][idx.c_str()]["text"];
    std::cout << "Processing entry: " << text << std::endl;

    //Generate the audio name based on the entry:
    auto filename = generate_persistant_filename(event,person,idx);
    //Remove the old audio file if existing
    std::cout << "  Removing old file" << std::endl;
    remove(filename.c_str());
    //Generate python command for audio download
    auto command = generate_command(filename,text);
    //Call python script to download the audio file
    std::cout << "  Downloading new file" << std::endl;
    system(command.c_str());
  }
}

void update_files(json& j,std::string event)
{
  //Update the files for Raul
  remove_and_download(j, event,"raul");
  //Update the files for Cris
  remove_and_download(j, event,"cris");
  //Update the files for Generic
  remove_and_download(j, event,"generic");
}


int main (int argc, const char *argv[])
{

  try
  {
    //Delete named mutex. Only in case of permanent block.
    //boost::interprocess::named_mutex::remove("my_mutex");

    //Command line variables
    std::string text_given;
    std::string audio_file;
    std::string event;
    std::string person;

    //Parse command line parameters
    options_description desc{"Options"};
    desc.add_options()
      ("help,h", "Help screen")
      ("text,t", value<std::string>(&text_given), "Text to translate to speech (TTS)")
      ("filename,f", value<std::string>(&audio_file), "Name of audio file to play")
      ("person,p", value<std::string>(&person), "Person assigned to the event [raul,cris,generic]")
      ("event,e", value<std::string>(&event), "Type of event [hello,goodbye,wakeup]")
      ("download,d", "Use this flag to download all audio files");

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    //Open or create the named mutex
    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, "my_mutex");

    //Parse JSON file
    std::ifstream i("messages_list.json");
    json j;
    i >> j;

    //Node logic

    //Show help
    if (vm.count("help"))
    {
      std::cout << desc << '\n';
    }

    else if(vm.count("download"))
    {
      std::cout << "Download option selected" << std::endl;
      update_files(j,"hello");
      update_files(j,"goodbye");
      update_files(j,"wakeup");
    }

    //If filename given, just play audio
    else if(vm.count("filename"))
    {
      play_audio(audio_file, std::ref(mutex));
    }

    //If text given, apply TTS and play downloaded audio
    else if(vm.count("text"))
    {
      //Generate filename
      auto tmp_filename = generate_tmp_filename();
      //Generate command for python script
      auto command = generate_command(tmp_filename, text_given);
      //Execute python script: TTS and audio download
      system(command.c_str());
      //Reproduce audio
      play_audio(tmp_filename, std::ref(mutex));
      //Remove audio
      remove(tmp_filename.c_str());
    }

    //If event type given, get audio information from JSON file
    else if (vm.count("event"))
    {
      //Generate random index to access the content of the JSON file
      auto idx = generate_random_index(j,event,person);

      //Get type of audio to reproduce (TTS or music)
      auto type_of_audio = j[event.c_str()][person.c_str()][idx.c_str()]["mode"];
      //Get text (TTS) or filename (music)
      auto text = j[event.c_str()][person.c_str()][idx.c_str()]["text"];

      //If TTS
      if(type_of_audio == "TTS")
      {
        std::cout << "Requested text to speech" << std::endl;
        //Generate filename
        auto filename = generate_persistant_filename(event, person, idx);
        //Reproduce audio
        play_audio(filename, std::ref(mutex));

      }
      //If music
      else if(type_of_audio == "music")
      {
        std::cout << "Requested audio play" << std::endl;
        //Reproduce audio
        play_audio(text, std::ref(mutex));
      }
      else
      {
        std::cout << "Wrong type of audio in JSON file" << std::endl;
      }
    }
  }

  catch(boost::interprocess::interprocess_exception &ex){
      std::cout << ex.what() << std::endl;
      return 1;
   }

  return 0;
}
