
#ifndef _sources_H
#define _sources_H

// Inspired by stormpkg, but a from-scratch implementation

#include <string>
#include <list>

using namespace std;

class SourcesList {
public:
	enum RecType { 
		Deb = 1 << 0, 
		DebSrc = 1 << 1, 
		Rpm = 1 << 2,
		RpmSrc = 1 << 3,
		Disabled = 1 << 4,
		Comment = 1 << 5
	};

	struct SourceRecord {
		unsigned char Type;
		string VendorID;
		string URI;
		string Dist;
		string *Sections;
		unsigned short NumSections;
		string Comment;
		string SourceFile;

		bool SetType(string);
		string GetType();
		bool SetURI(string);

		SourceRecord() : Type(0), Sections(0), NumSections(0) {};
		~SourceRecord() { if (Sections) delete[] Sections; };
		SourceRecord &operator=(const SourceRecord &);
	};

	struct VendorRecord
	{
		string VendorID;
		string FingerPrint;
		string Description;
	};

	list<SourceRecord*> SourceRecords;
	list<VendorRecord*> VendorRecords;

private:
	SourceRecord* AddSourceNode(SourceRecord &);
	VendorRecord* AddVendorNode(VendorRecord &);

public:
	SourceRecord *AddSource(RecType Type,
				string VendorID,
				string URI,
				string Dist,
				string *Sections,
				unsigned short count,
				string SourceFile);
	SourceRecord *AddEmptySource();
	void RemoveSource(SourceRecord *&);
	bool ReadSourcePart(string listpath);
	bool ReadSourceDir(string Dir);
	bool ReadSources();
	bool UpdateSources();

	VendorRecord *AddVendor(string VendorID,
				string FingerPrint,
				string Description);
	void RemoveVendor(VendorRecord *&);
	bool ReadVendors();
	bool UpdateVendors();

	SourcesList() {};
	~SourcesList();
};

ostream &operator <<(ostream &, const SourcesList::SourceRecord &);

#endif
