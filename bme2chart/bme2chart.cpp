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
		Key(int a, int b) {
			tick = a;
			colour = b;
		}

		Key() {
			tick = 0;
			colour = 0;
		}
		
		int getTick() { return tick; }
		int getColour() { return colour; }

	private:
		int tick;
		int colour;
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
		cerr << "Usage: bme2chart.exe <input path> <output path>"
			<< endl
			<< endl
			<< "    <output path> is purely optional."
			<< endl
			<< "    If blank, uses input name + \".chart\" (not affecting the bme!)"
			<< endl;
		return 1;
	}

	string path, opath;

	if (argc == 2) {
		path = argv[1];
		opath = string(argv[1]) + ".chart";
	}

	if (argc == 3) {
		path = argv[1];
		opath = argv[2];
	}

	bme.open(path);

	//Song Data
	string sname = "", sartist = "", sgenre = "";
	string line, prtln, bfln, afln, attln, bycol;

	vector<Event> Events;
	vector<AUDIO> Audio;

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
			else if (bfln == "TITLE")
				sname = afln;
			else if (bfln == "ARTIST")
				sartist = afln;
			else if (bfln == "GENRE")
				sgenre = afln;
			else if (bfln == "PLAYLEVEL" || bfln == "BPM" || bfln == "PLAYER") {
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
	song_str += "\tMusicStream = \"song.ogg\"\n";
	song_str += "}\n";

	//Loop through all of the events and export
	string snctrk_str = "[SyncTrack]\n";
	snctrk_str += "{\n";
	snctrk_str += "\t0 = TS 4\n";
	for (int i = 0; i < Events.size(); i++) {
		if (Events[i].getType() == 3) {
			vector<string> breakup;
			Events[i].convertToSegments(breakup);
			for (int a = 0; a < breakup.size(); a++) {
				if (breakup[a] != "00") {
					snctrk_str += "\t" + ToString(calcTick(Events[i].getMeasure(), resolution, a, Events[i].getEntryCount())) + " = B " + ToString(HexToInt(breakup[a]) * 1000) + "\n";
				}
			}
		}
	}
	snctrk_str += "}\n";

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
					vkey.push_back(Key(calcTick(Events[i].getMeasure(), resolution, a, Events[i].getEntryCount()), Events[i].getNote(fivekeymode)));
				}
			}
		}
	}
	merge_sort(vkey, 0, vkey.size() - 1);


	//Scan for Note Data now!
	for (int i = 0; i < vkey.size(); i++) {
		note_str += "\t" + ToString(vkey[i].getTick()) + " = N " + ToString(vkey[i].getColour()) + " 0\n";
	}
	
	note_str += "}\n";

	ofstream chart;
	chart.open(opath);
	chart << song_str << snctrk_str << note_str;
	chart.close();
}