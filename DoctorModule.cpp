#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <optional>
#include <stdexcept>
#include <filesystem>

#include "IndexManagers.h"

using namespace std;

// Global index manager for doctors
DoctorIndexManager docIndexMgr;



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
    char status[DOC_STATUS_LEN];
};

// =====================================================================
//                    HELPER FUNCTIONS
// =====================================================================

void DoctorWriteFixed(char* dest, const string& s, int size) 
{
    memset(dest, 0, size);
    for (int i = 0; i < size && i < (int)s.length(); ++i)
        dest[i] = s[i];
}

string DoctorReadFixed(const char* src, int size) 
{
    int len = 0;
    while (len < size && src[len] != 0) len++;
    return string(src, len);
}

// =====================================================================
//                    FILE I/O FUNCTIONS
// =====================================================================

long appendDoctorRecord(const DoctorRecord& rec)
{
    fstream file(DOC_DATA_FILE, ios::in | ios::out | ios::binary);

    if (!file.is_open())
    {
        ofstream createFile(DOC_DATA_FILE, ios::out | ios::app | ios::binary);
        createFile.close();

        file.open(DOC_DATA_FILE, ios::in | ios::out | ios::binary);
    }

    file.seekp(0, ios::end);
    long pos = file.tellp() / sizeof(DoctorRecord);

    file.write(reinterpret_cast<const char*>(&rec), sizeof(DoctorRecord));
    file.close();

    return pos;
}


void writeDoctorRecord(long pos, const DoctorRecord& rec)
{
    fstream file(DOC_DATA_FILE, ios::in | ios::out | ios::binary);
    if (!file.is_open()) throw runtime_error("Cannot open doctors.dat");

    file.seekp(pos * sizeof(DoctorRecord), ios::beg);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(DoctorRecord));
    file.close();
}

DoctorRecord readDoctorRecord(long pos) 
{
    ifstream file(DOC_DATA_FILE, ios::binary);
    if (!file.is_open()) throw runtime_error("Cannot open doctors.dat");

    file.seekg(pos * sizeof(DoctorRecord), ios::beg);

    DoctorRecord rec;
    if (!file.read(reinterpret_cast<char*>(&rec), sizeof(DoctorRecord)))
        throw runtime_error("Failed to read doctor record");

    file.close();
    return rec;
}

// =====================================================================
//                    DOCTOR MANAGER
// =====================================================================

class DoctorManager
{
public:

    void AddDoctor(const string& id, const string& name, const string& addr)
    {
        // Duplicate check
        if (docIndexMgr.searchByPrimary(id))
        {
            cout << "Doctor ID already exists.\n";
            return;
        }

        // Build record
        DoctorRecord rec;
        DoctorWriteFixed(rec.doctor_id, id, DOC_ID_LEN);
        DoctorWriteFixed(rec.doctor_name, name, DOC_NAME_LEN);
        DoctorWriteFixed(rec.address, addr, DOC_ADDRESS_LEN);
        DoctorWriteFixed(rec.status, "Active", DOC_STATUS_LEN);

        // Write to file
        long pos = appendDoctorRecord(rec);

        // INSERT INTO PRIMARY
        int indexPos = docIndexMgr.insertPrimary(id, pos);

        // INSERT INTO SECONDARY (based on name)
        docIndexMgr.insertSecondary(name, indexPos);
    }


    void UpdateDoctorName(const string& id, const string& new_name)
    {
        const DocPrimaryIndexEntry* entry = docIndexMgr.searchByPrimary(id);

        if (!entry)
        {
            cout << "Doctor not found.\n";
            return;
        }

        long pos = entry->offset;
        DoctorRecord rec = readDoctorRecord(pos);

        if (DoctorReadFixed(rec.status, DOC_STATUS_LEN) != "Active")
        {
            cout << "Cannot update deleted doctor.\n";
            return;
        }

        string oldName = DoctorReadFixed(rec.doctor_name, DOC_NAME_LEN);

        // Update secondary index
        docIndexMgr.deleteSecondary(oldName, pos);
        docIndexMgr.insertSecondary(new_name, pos);

        // Update file record
        DoctorWriteFixed(rec.doctor_name, new_name, DOC_NAME_LEN);
        writeDoctorRecord(pos, rec);
    }


    void DeleteDoctor(const string& id)
    {
        const DocPrimaryIndexEntry* entry = docIndexMgr.searchByPrimary(id);

        if (!entry)
        {
            cout << "Doctor not found.\n";
            return;
        }

        long pos = entry->offset;
        DoctorRecord rec = readDoctorRecord(pos);

        if (DoctorReadFixed(rec.status, DOC_STATUS_LEN) == "Deleted")
        {
            cout << "Doctor already deleted.\n";
            return;
        }

        string doctorName = DoctorReadFixed(rec.doctor_name, DOC_NAME_LEN);

        // Update secondary index
        docIndexMgr.deleteSecondary(doctorName, pos);

        // Mark record as deleted
        DoctorWriteFixed(rec.status, "Deleted", DOC_STATUS_LEN);
        writeDoctorRecord(pos, rec);

        // Remove from primary index
        docIndexMgr.deletePrimary(id);
    }


    optional<DoctorRecord> getByDoctorId(const string& id)
    {
        const DocPrimaryIndexEntry* entry = docIndexMgr.searchByPrimary(id);

        if (!entry) return nullopt;

        DoctorRecord rec = readDoctorRecord(entry->offset);

        if (DoctorReadFixed(rec.status, DOC_STATUS_LEN) == "Active")
            return rec;

        return nullopt;
    }

    vector<DoctorRecord> getByDoctorName(const string& name)
    {
        vector<DoctorRecord> result;

        auto entries = docIndexMgr.searchBySecondary(name);
        for (auto entry : entries)
        {
            DoctorRecord rec = readDoctorRecord(entry->offset);
            if (DoctorReadFixed(rec.status, DOC_STATUS_LEN) == "Active")
                result.push_back(rec);
        }

        return result;
    }


    static void printRecord(const DoctorRecord& rec)
{
        cout << "DoctorID: " << DoctorReadFixed(rec.doctor_id, DOC_ID_LEN)
            << " | Name: " << DoctorReadFixed(rec.doctor_name, DOC_NAME_LEN)
            << " | Address: " << DoctorReadFixed(rec.address, DOC_ADDRESS_LEN)
            << " | Status: " << DoctorReadFixed(rec.status, DOC_STATUS_LEN)
            << "\n";
    }
};
