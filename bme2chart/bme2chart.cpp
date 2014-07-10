/*
		BME2CHART C++
		Because Beatmania's clever format can also be in chart

		The purpose of this console application is to convert Beatmania BME files to chart format. It can then be converted to MIDI via chart2mid.

		By: Clara Eleanor Taylor
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

class Event {
	public:
		//Constructors
		Event(int a, int b, string c) {
			measure = a;
			type    = b;
			data    = c;
		}
		Event() {
			measure = 0;
			type    = 0;
			data    = "";
		}
		
		//Return thingies...
		int getMeasure() { return measure; }
		int getType() { return type; }
		string getData() { return data; }
		int getEntryCount() { return data.length() / 2; }
		void convertToSegments(vector<string> &thing) {
			thing.clear();
			int size = data.length() / 2;
			for (int i = 0; i < size; i++) {
				thing.push_back(data.substr(i * 2, 2));
			}
		}
		bool getIsNote() {
			return ((type >= 11 && type <= 16) || type == 18 || type == 19);
		}
		int getNote(bool fivekeymode) {
			switch (type) {
				case 11: return 0;
				case 12: return 1;
				case 13: return 2;
				case 14: return 3;
				case 15: return 4;
				case 18: 
					if (fivekeymode)
						return 3;
					else
						return 5;
				case 19: 
					if (fivekeymode)
						return 4;
					else
						return 6;
				case 16: 
					if (fivekeymode)
						return 0;
					else
						return 7;
				default: return -1;
			}
		}
	private:
		int measure;
		int type;
		string data;
};

class Key {
	public:
		Key(int a, int b, int c) {
			tick = a;
			colour = b;
			sound = c;
		}

		Key() {
			tick = 0;
			colour = 0;
			sound = 0;
		}
		
		int getTick() { return tick; }
		int getColour() { return colour; }
		int getSound() { return sound; }

	private:
		int tick;
		int colour;
		int sound;
};

class AUDIO {
	public:
		//Constructors
		AUDIO(string a, string b) {
			attr = a;
			path = b;
		}
		AUDIO() {
			attr = "";
			path = "";
		}
		
		//Return thingies...
		string getAttribute() { return attr; }
		string getPath() { return path; }
	private:
		string attr;
		string path;
};

class BPM {
	public:
		//Constructors
		BPM(string a, int b) {
			attr = a;
			bpm = b;
		}
		BPM() {
			attr = "";
			bpm = 0;
		}
		
		//Return thingies...
		string getAttribute() { return attr; }
		int getBPM() { return bpm; }
	private:
		string attr;
		int bpm;
};

int findSound(vector<AUDIO> sndbk, string ID) {
	for (int i = 0; i < sndbk.size(); i++)
		if (ID == sndbk[i].getAttribute())
			return i;
	return 0;
}

int ToReal(string str) {
	istringstream output;
	output.clear();
	output.str(str);
	int i;
	output >> i;
	return i;
}

int HexToInt(string str) {
	unsigned int x;
	stringstream ss;
	ss << hex << str;
	ss >> x;
	return x;
}

string ToString(int integer) {
	string res;
	ostringstream convert;
	convert << integer;
	return convert.str();
}

int calcTick(int measure, int resolution, int pos, int length) {
	return (measure * (resolution * 4)) + ((resolution * 4) * ((double)pos / length));
}

void merge(vector<Key> &vec, int p, int r) {
	int mid = floor((double)(p + r) / 2);
	int i1 = 0;
	int i2 = p;
	int i3 = mid + 1;

	//Temporary Vector
	vector<Key> tmp;
	
	//Merge in sorted form the 2 arrays
	while ( i2 <= mid && i3 <= r )
        if ( vec[i2].getTick() < vec[i3].getTick() )
            tmp.push_back(vec[i2++]);
        else
            tmp.push_back(vec[i3++]);

    while ( i2 <= mid )
        tmp.push_back(vec[i2++]);

    while ( i3 <= r )
        tmp.push_back(vec[i3++]);

    for ( int i = p; i <= r; i++ )
        vec[i] = tmp[i-p];
}

void merge_sort(vector<Key> &vec, int p, int r) {
	if (p < r) {
		int mid = floor((double)(p + r) / 2);
		merge_sort(vec, p, mid); //RECURSIVE!
		merge_sort(vec, mid + 1, r);
		merge(vec, p, r);
	}
}

int main(int argc, char* argv[]) {

	cout << "/---------------------------------------------------------\\" << endl
		<< "|*                                                       *|" << endl
		<< "|                  < Clara's bme2chart >                  |" << endl
		<< "|                     Version 1.0.0.0                     |" << endl
		<< "|                                                         |" << endl
		<< "|    For people who know what the hell they are doing.    |" << endl
		<< "|*                                                       *|" << endl
		<< "\\---------------------------------------------------------/\n\n";

	bool keyboard_mode = false; //Export to ExpertKeyboard rather than ExpertBeatmania.
	bool fivekeymode = false; //Yeah why not?

	ifstream bme;

	if (argc < 2) {
		cerr << "Usage: bme2chart.exe <input path> <output path> <soundbank path>"
			<< endl
			<< endl
			<< "    <output path> is purely optional, as well as soundbank path."
			<< endl
			<< "    If blank, uses input name + \".chart\" (not affecting the bme!), same with soundbank, only with \".xsd\""
			<< endl;
		return 1;
	}

	string path, opath, spath;

	if (argc == 2) {
		path = argv[1];
		opath = string(argv[1]) + ".chart";
		spath = string(argv[1]) + ".xsd";
	}

	if (argc == 3) {
		path = argv[1];
		opath = argv[2];
		spath = string(argv[2]) + ".xsd";
	}

	if (argc == 4) {
		path = argv[1];
		opath = argv[2];
		spath = argv[3];
	}

	bme.open(path);

	//Song Data
	string sname = "", sartist = "", sgenre = "";
	string line, prtln, bfln, afln, attln, bycol;
	unsigned int bpmf = 0;

	vector<Event> Events;
	vector<AUDIO> Audio;
	vector<BPM> AdvBPM;

	while (getline(bme,line)) {
		if (line.length() == 0)
			continue; //Yeah... might as well.
		if (line[0] == '#') {
			//EVENT!
			prtln = line.substr(1, 3);
			attln = line.substr(4, 2);
			bycol = line.substr(7, line.length() - 7);
			bfln  = line.substr(1, line.find(' ') - 1);
			afln  = line.substr(line.find(' ') + 1, line.length() - (line.find(' ') + 1));
			if (prtln == "WAV")
				Audio.push_back(AUDIO(attln, bycol));
			else if (bfln == "BPM")
				bpmf = ToReal(afln);
			else if (prtln == "BPM") {
				AdvBPM.push_back(BPM(attln, ToReal(bycol)));
			}
			else if (bfln == "TITLE")
				sname = afln;
			else if (bfln == "ARTIST")
				sartist = afln;
			else if (bfln == "GENRE")
				sgenre = afln;
			else if (bfln == "PLAYLEVEL" || bfln == "PLAYER") {
				//Do nothing
			} else
				//The rest must be numbers.
				Events.push_back(Event(ToReal(prtln), ToReal(attln), bycol));
		}
	}

	//We no longer need this.
	bme.close();

	int resolution = 960;

	//Write [Song]
	string song_str = "[Song]\n";
	song_str += "{\n";
	song_str += "\tName = \"" + sname + "\"\n";
	song_str += "\tArtist = \"" + sartist + "\"\n";
	song_str += "\tCharter = bme2chart\n";
	song_str += "\tOffset = 0\n";
	song_str += "\tResolution = 960\n";
	song_str += "\tPlayer2 = bass\n";
	song_str += "\tDifficulty = 0\n";
	song_str += "\tPreviewStart = 0.00\n";
	song_str += "\tPreviewEnd = 0.00\n";
	song_str += "\tGenre = \"" + sgenre + "\"\n";
	song_str += "\tMediaType = \"cd\"\n";
	song_str += "\tMusicStream = \"01.ogg\"\n";
	song_str += "}\n";

	//Loop through all of the events and export
	string snctrk_str = "[SyncTrack]\n";
	snctrk_str += "{\n";
	snctrk_str += "\t0 = TS 4\n";
	snctrk_str += "\t0 = B " + ToString(bpmf * 1000) + "\n";
	for (int i = 0; i < Events.size(); i++) {
		if (Events[i].getType() == 3) {
			//Regular BPM Change
			vector<string> breakup;
			Events[i].convertToSegments(breakup);
			for (int a = 0; a < breakup.size(); a++) {
				if (breakup[a] != "00") {
					snctrk_str += "\t" + ToString(calcTick(Events[i].getMeasure(), resolution, a, Events[i].getEntryCount())) + " = B " + ToString(HexToInt(breakup[a]) * 1000) + "\n";
				}
			}
		}
		if (Events[i].getType() == 8) {
			//Advanced BPM Change
			vector<string> breakup;
			Events[i].convertToSegments(breakup);
			for (int a = 0; a < breakup.size(); a++) {
				if (breakup[a] != "00") {
					for (int b = 0; b < AdvBPM.size(); b++)
						if (breakup[a] == AdvBPM[b].getAttribute())
							snctrk_str += "\t" + ToString(calcTick(Events[i].getMeasure(), resolution, a, Events[i].getEntryCount())) + " = B " + ToString(AdvBPM[b].getBPM() * 1000) + "\n";
					//If you write the BME file correctly, it is guaranteed to spit something out here, period.
				}
			}
		}
	}
	snctrk_str += "}\n";

	string event_str = "[Events]\n{\n}\n"; //LOL

	string note_str;
	if (keyboard_mode)
		note_str = "[ExpertKeyboard]\n";
	else
		note_str = "[ExpertBeatmania]\n";
	note_str += "{\n";

	//Notes require an extra step due to the fact that they can be out of order. We need to sort them out.
	vector<Key> vkey;
	for (int i = 0; i < Events.size(); i++) {
		if (Events[i].getIsNote()) {
			vector<string> breakup;
			Events[i].convertToSegments(breakup);
			for (int a = 0; a < breakup.size(); a++) {
				if (breakup[a] != "00") {
					vkey.push_back(Key(calcTick(Events[i].getMeasure(), resolution, a, Events[i].getEntryCount()), Events[i].getNote(fivekeymode), findSound(Audio, breakup[a])));
				}
			}
		}
	}
	merge_sort(vkey, 0, vkey.size() - 1);


	//Scan for Note Data now!
	for (int i = 0; i < vkey.size(); i++) {
		note_str += "\t" + ToString(vkey[i].getTick()) + " = N " + ToString(vkey[i].getColour()) + " 0 " + ToString(vkey[i].getSound()) + "\n";
	}
	
	note_str += "}\n";

	//Write Chart
	ofstream chart;
	chart.open(opath);
	chart << song_str << snctrk_str << event_str << note_str;
	chart.close();

	//Write Soundbank
	ofstream sb;
	sb.open(spath);
	for (int i = 0; i < Audio.size(); i++)
		sb << Audio[i].getPath() << endl;
	sb.close();
}