#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

const int DOC_ID_LEN = 15;
const int DOC_NAME_LEN = 30;
const int DOC_ADDRESS_LEN = 30;
const int DOC_DATA_LEN = DOC_ID_LEN + DOC_NAME_LEN + DOC_ADDRESS_LEN;
const int DOC_RECORD_LEN = 1 + 3 + DOC_DATA_LEN;
const string DOC_FILE_NAME = "Doctors.dat";

//DEPENDENCY STUBS (will implemented by Ahmed)
short getDoctorAvailSlot(short record_size)
{
    return -1;
}
void addDoctorToAvailList(short offset, short record_size)
{
    cout << "STUB: Added offset " << offset << " to AVAIL LIST." << endl;
}
short searchDoctorPrimaryIndex(const char* id)
{
    return -1;
}
void insertDoctorIndex(const char* index_name, const char* key, short offset)
{
    cout << "STUB: Inserted key " << key << " into " << index_name << " at offset " << offset << endl;
}
void deleteDoctorIndex(const char* index_name, const char* key, short offset)
{
    cout << "STUB: Deleted key " << key << " from " << index_name << " at offset " << offset << endl;
}

struct DoctorRecord
{
    char ID[DOC_ID_LEN + 1];
    char Name[DOC_NAME_LEN + 1];
    char Address[DOC_ADDRESS_LEN + 1];

    DoctorRecord(const char* id = "", const char* name = "", const char* address = "")
    {
        strncpy(ID, id, DOC_ID_LEN);
        ID[DOC_ID_LEN] = '\0';
        strncpy(Name, name, DOC_NAME_LEN);
        Name[DOC_NAME_LEN] = '\0';
        strncpy(Address, address, DOC_ADDRESS_LEN);
        Address[DOC_ADDRESS_LEN] = '\0';
    }

    string toRecordString(char marker = ' ') const
    {
        stringstream ss;
        ss << marker;
        ss << "075";
        ss << left << setw(DOC_ID_LEN) << string(ID).substr(0, DOC_ID_LEN);
        ss << left << setw(DOC_NAME_LEN) << string(Name).substr(0, DOC_NAME_LEN);
        ss << left << setw(DOC_ADDRESS_LEN) << string(Address).substr(0, DOC_ADDRESS_LEN);
        return ss.str();
    }

    static DoctorRecord fromRecordString(const string& record_data)
    {
        string data_part = record_data.substr(4);
        string id_str = data_part.substr(0, DOC_ID_LEN);
        string name_str = data_part.substr(DOC_ID_LEN, DOC_NAME_LEN);
        string address_str = data_part.substr(DOC_ID_LEN + DOC_NAME_LEN, DOC_ADDRESS_LEN);

        DoctorRecord doc;
        strncpy(doc.ID, id_str.c_str(), DOC_ID_LEN);
        doc.ID[DOC_ID_LEN] = '\0';
        strncpy(doc.Name, name_str.c_str(), DOC_NAME_LEN);
        doc.Name[DOC_NAME_LEN] = '\0';
        strncpy(doc.Address, address_str.c_str(), DOC_ADDRESS_LEN);
        doc.Address[DOC_ADDRESS_LEN] = '\0';
        return doc;
    }
};


void initDoctorFile()
{
    fstream file(DOC_FILE_NAME, ios::out | ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: Could not open " << DOC_FILE_NAME << " for initialization." << endl;
        return;
    }
    short header = -1;
    file.write((char*)&header, sizeof(header));
    file.close();
    cout << DOC_FILE_NAME << " initialized with empty AVAIL LIST header." << endl;
}

void writeDoctorRecord(const DoctorRecord& doc, short offset, char marker = ' ')
{
    fstream file(DOC_FILE_NAME, ios::in | ios::out | ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: Could not open " << DOC_FILE_NAME << " for writing." << endl;
        return;
    }
    file.seekp(offset, ios::beg);
    string record_str = doc.toRecordString(marker);
    file.write(record_str.c_str(), DOC_RECORD_LEN);
    file.close();
}

short appendDoctorRecord(const DoctorRecord& doc) {
    fstream file(DOC_FILE_NAME, ios::in | ios::out | ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: Could not open " << DOC_FILE_NAME << " for appending." << endl;
        return -1;
    }
    file.seekp(0, ios::end);
    short offset = static_cast<short>(file.tellp());
    string record_str = doc.toRecordString();
    file.write(record_str.c_str(), DOC_RECORD_LEN);
    file.close();
    return offset;
}


void AddDoctorRecord(const DoctorRecord& new_doc)
{
    if (searchDoctorPrimaryIndex(new_doc.ID) != -1)
    {
        cout << "Warning: Doctor with ID " << new_doc.ID << " already exists. Record not added." << endl;
        return;
    }

    short offset = getDoctorAvailSlot(DOC_RECORD_LEN);
    if (offset != -1)
    {
        writeDoctorRecord(new_doc, offset);
        cout << "Doctor added to free slot at offset: " << offset << endl;
    }
    else
    {
        offset = appendDoctorRecord(new_doc);
        if (offset == -1) return;
        cout << "Doctor appended to file at offset: " << offset << endl;
    }

    insertDoctorIndex("DoctorIDPrimary", new_doc.ID, offset);
    insertDoctorIndex("DoctorNameSecondary", new_doc.Name, offset);
}

void UpdateDoctorName(const char* id, const char* new_name)
{
    short offset = searchDoctorPrimaryIndex(id);
    if (offset == -1)
    {
        cout << "Warning: Doctor with ID " << id << " not found. Update failed." << endl;
        return;
    }
    if (strlen(new_name) > DOC_NAME_LEN)
    {
        cout << "Error: New name exceeds allocated size." << endl;
        return;
    }

    fstream file(DOC_FILE_NAME, ios::in | ios::out | ios::binary);
    file.seekg(offset, ios::beg);
    char buffer[DOC_RECORD_LEN + 1];
    file.read(buffer, DOC_RECORD_LEN);
    buffer[DOC_RECORD_LEN] = '\0';
    DoctorRecord old_doc = DoctorRecord::fromRecordString(buffer);

    deleteDoctorIndex("DoctorNameSecondary", old_doc.Name, offset);

    DoctorRecord new_doc = old_doc;
    strncpy(new_doc.Name, new_name, DOC_NAME_LEN);
    new_doc.Name[DOC_NAME_LEN] = '\0';

    file.seekp(offset, ios::beg);
    string record_str = new_doc.toRecordString();
    file.write(record_str.c_str(), DOC_RECORD_LEN);
    file.close();

    insertDoctorIndex("DoctorNameSecondary", new_doc.Name, offset);
    cout << "Doctor ID " << id << " name updated from " << old_doc.Name << " to " << new_doc.Name << endl;
}

void DeleteDoctorRecord(const char* id)
{
    short offset = searchDoctorPrimaryIndex(id);
    if (offset == -1)
    {
        cout << "Warning: Doctor with ID " << id << " not found. Delete failed." << endl;
        return;
    }

    fstream file(DOC_FILE_NAME, ios::in | ios::out | ios::binary);
    file.seekg(offset, ios::beg);
    char buffer[DOC_RECORD_LEN + 1];
    file.read(buffer, DOC_RECORD_LEN);
    buffer[DOC_RECORD_LEN] = '\0';
    DoctorRecord doc_to_delete = DoctorRecord::fromRecordString(buffer);
    file.close();

    writeDoctorRecord(doc_to_delete, offset, '*');
    cout << "Doctor ID " << id << " soft-deleted at offset: " << offset << endl;

    addDoctorToAvailList(offset, DOC_RECORD_LEN);
    deleteDoctorIndex("DoctorIDPrimary", doc_to_delete.ID, offset);
    deleteDoctorIndex("DoctorNameSecondary", doc_to_delete.Name, offset);
}

//main just for testing
int main()
{
    initDoctorFile();

    DoctorRecord d1("D001", "Dr. Smith", "123 Main St");
    DoctorRecord d2("D002", "Dr. Jones", "456 Oak Ave");
    DoctorRecord d3("D003", "Dr. Brown", "789 Pine Ln");

    AddDoctorRecord(d1);
    AddDoctorRecord(d2);
    AddDoctorRecord(d3);

    UpdateDoctorName("D002", "Dr. Jones-New");
    DeleteDoctorRecord("D001");
    AddDoctorRecord(d3);
    DeleteDoctorRecord("D999");

    return 0;
}
