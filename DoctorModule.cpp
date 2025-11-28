#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <optional>
#include "IndexManagers.h"

using namespace std;

const int ID_LEN = 15;
const int NAME_LEN = 30;
const int ADDRESS_LEN = 30;
const int DATA_LEN = ID_LEN + NAME_LEN + ADDRESS_LEN;

const string DOC_DATA_FILE = "doctors.dat";
const int DOC_ID_LEN = 15;
const int DOC_NAME_LEN = 30;
const int DOC_ADDRESS_LEN = 30;
const int DOC_STATUS_LEN = 8;

struct DoctorRecord
{
    char doctor_id[DOC_ID_LEN];
    char doctor_name[DOC_NAME_LEN];
    char address[DOC_ADDRESS_LEN];
    char status[DOC_STATUS_LEN]; // "Active" or "Deleted"
};

// Global instance of the DoctorIndexManager (from IndexManagers.h)
extern DoctorIndexManager docIndexMgr;

short getDoctorAvailSlot(size_t record_size)
{
    return -1;
}

void addDoctorToAvailList(short offset, size_t record_size)
{
    cout << "--- Mock: Added record at disk offset " << offset << " to Doctor Avail List ---\n";
}


void writeFixed(char* dest, const string& s, int size)
{
    memset(dest, 0, size);
    for (int i = 0; i < size && i < (int)s.length(); ++i)
    {
        dest[i] = s[i];
    }
}
string readFixed(const char* src, int size)
{
    int len = 0;
    while (len < size && src[len] != 0) len++;
    return string(src, len);
}

long appendRecord(const DoctorRecord& rec)
{
    fstream file(DOC_DATA_FILE, ios::in | ios::out | ios::binary);
    if (!file.is_open())
    {
        file.open(DOC_DATA_FILE, ios::out | ios::binary);
        file.close();
        file.open(DOC_DATA_FILE, ios::in | ios::out | ios::binary);
    }
    file.seekp(0, ios::end);
    long pos = file.tellp() / sizeof(DoctorRecord);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(DoctorRecord));
    file.close();
    return pos;
}

void writeRecord(long pos, const DoctorRecord& rec)
{
    fstream file(DOC_DATA_FILE, ios::in | ios::out | ios::binary);
    if (!file.is_open()) throw runtime_error("Cannot open data file");
    file.seekp(pos * sizeof(DoctorRecord), ios::beg);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(DoctorRecord));
    file.close();
}

DoctorRecord readRecord(long pos)
{
    ifstream file(DOC_DATA_FILE, ios::binary);
    if (!file.is_open()) throw runtime_error("Cannot open data file");
    file.seekg(pos * sizeof(DoctorRecord), ios::beg);
    DoctorRecord rec;
    if (!file.read(reinterpret_cast<char*>(&rec), sizeof(DoctorRecord)))
    {
        throw runtime_error("Failed to read record at position " + to_string(pos));
    }
    file.close();
    return rec;
}

void init_doctor_file()
{
}


class DoctorManager
{
public:
    // 2.1: Add Doctor
    void AddDoctor(const string& docId, const string& name, const string& address)
    {
        // 1. Check for Duplicates
        if (docIndexMgr.searchByPrimary(docId))
        {
            cout << "Warning: Doctor with ID " << docId << " already exists. Record not added." << endl;
            return;
        }

        DoctorRecord rec;
        writeFixed(rec.doctor_id, docId, DOC_ID_LEN);
        writeFixed(rec.doctor_name, name, DOC_NAME_LEN);
        writeFixed(rec.address, address, DOC_ADDRESS_LEN);
        writeFixed(rec.status, "Active", DOC_STATUS_LEN);

        // 2. Check AVAIL LIST (using mock function)
        long pos;
        size_t record_size = sizeof(DoctorRecord);
        pos = getDoctorAvailSlot(record_size);
        
        if (pos != -1)
        {
            writeRecord(pos, rec);
        } 
        else
        {
            pos = appendRecord(rec);
        }

        // 3. Update Indexes
        int primaryPos = docIndexMgr.insertPrimary(docId, (short)pos);
        docIndexMgr.insertSecondary(name, primaryPos);
    }

    // 2.2: Update Doctor Name
    void UpdateDoctorName(const string& docId, const string& new_name)
    {
        // 1. Search for the record offset
        const DocPrimaryIndexEntry* entry = docIndexMgr.searchByPrimary(docId);
        if (!entry)
        {
            cout << "Warning: Doctor with ID " << docId << " not found. Update failed." << endl;
            return;
        }
        long pos = entry->offset;

        // Check size constraint
        if (new_name.length() > DOC_NAME_LEN)
        {
            cout << "Error: New name exceeds allocated size (" << DOC_NAME_LEN << " chars). Update failed." << endl;
            return;
        }

        // 2. Read the old record
        DoctorRecord rec = readRecord(pos);
        
        // Check if deleted
        if (readFixed(rec.status, DOC_STATUS_LEN) != "Active")
        {
            cout << "Cannot update deleted doctor record.\n";
            return;
        }

        // 3. Update Secondary Index (Delete old entry)
        string old_name = readFixed(rec.doctor_name, DOC_NAME_LEN);
        int primaryPos = docIndexMgr.binarySearchPrimary(docId);
        if (primaryPos != -1)
        {
            docIndexMgr.deleteSecondary(old_name, primaryPos);
        }

        // 4. Modify the record and write back
        writeFixed(rec.doctor_name, new_name, DOC_NAME_LEN);
        writeRecord(pos, rec);
        
        cout << "Doctor ID " << docId << " name updated from " << old_name << " to " << new_name << endl;

        // 5. Update Secondary Index (Insert new entry)
        if (primaryPos != -1)
        {
            docIndexMgr.insertSecondary(new_name, primaryPos);
        }
    }

    // 2.3: Delete Doctor
    void DeleteDoctor(const string& docId)
    {
        // 1. Search for the record offset
        const DocPrimaryIndexEntry* entry = docIndexMgr.searchByPrimary(docId);
        if (!entry)
        {
            cout << "Warning: Doctor with ID " << docId << " not found. Delete failed." << endl;
            return;
        }
        long pos = entry->offset;

        // 2. Read the record
        DoctorRecord rec = readRecord(pos);
        
        // Check if already deleted
        if (readFixed(rec.status, DOC_STATUS_LEN) == "Deleted")
        {
            cout << "Doctor ID " << docId << " is already deleted.\n";
            return;
        }

        // 3. Soft Delete (Update status field)
        writeFixed(rec.status, "Deleted", DOC_STATUS_LEN);
        writeRecord(pos, rec);
        cout << "Doctor ID " << docId << " soft-deleted at position: " << pos << endl;

        // 4. Update AVAIL LIST (using mock function)
        addDoctorToAvailList((short)pos, sizeof(DoctorRecord));

        // 5. Update Indexes
        string name_to_delete = readFixed(rec.doctor_name, DOC_NAME_LEN);
        int deletedPos = docIndexMgr.deletePrimary(docId);
        
        if (deletedPos != -1)
        {
            docIndexMgr.deleteSecondary(name_to_delete, deletedPos);
        }
    }

    // Helper to print a single Doctor record
    static void printRecord(const DoctorRecord& rec)
    {
        cout << "Doctor ID: " << readFixed(rec.doctor_id, DOC_ID_LEN)
             << " | Name: " << readFixed(rec.doctor_name, DOC_NAME_LEN)
             << " | Address: " << readFixed(rec.address, DOC_ADDRESS_LEN)
             << " | Status: " << readFixed(rec.status, DOC_STATUS_LEN) << "\n";
    }

    // Helper to get a single Doctor record by ID (used by QueryManager and Print Info)
    optional<DoctorRecord> getByDoctorId(const string& docId)
    {
        const DocPrimaryIndexEntry* entry = docIndexMgr.searchByPrimary(docId);

        if (!entry)
        {
            return nullopt;
        }

        DoctorRecord rec = readRecord(entry->offset);
        
        if (readFixed(rec.status, DOC_STATUS_LEN) == "Active")
        {
            return rec;
        }

        return nullopt;
    }

    // Helper to get Doctor records by Name (used by QueryManager)
    vector<DoctorRecord> getByDoctorName(const string& docName)
    {
        vector<DoctorRecord> result;

        vector<const DocPrimaryIndexEntry*> primaryEntries = docIndexMgr.searchBySecondary(docName);

        for (const auto* entry : primaryEntries)
        {
            DoctorRecord rec = readRecord(entry->offset);
            
            if (readFixed(rec.status, DOC_STATUS_LEN) == "Active")
            {
                result.push_back(rec);
            }
        }
        return result;
    }
};
