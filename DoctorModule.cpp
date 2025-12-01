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

class AvailListManager
{
private:
    const string data_file_name;
    const string avail_file_name;
    const size_t record_size;
    long head_position;

    void readHead()
    {
        ifstream file(avail_file_name, ios::binary);
        if (!file.is_open())
        {
            head_position = -1;
            return;
        }
        file.read(reinterpret_cast<char*>(&head_position), sizeof(long));
        file.close();
    }

    void writeHead() const
    {
        ofstream file(avail_file_name, ios::binary | ios::trunc);
        if (!file.is_open())
        {
            throw runtime_error("Cannot open AVAIL LIST file for writing: " + avail_file_name);
        }
        file.write(reinterpret_cast<const char*>(&head_position), sizeof(long));
        file.close();
    }

public:
    AvailListManager(const string& data_file, size_t rec_size)
            : data_file_name(data_file),
              avail_file_name(data_file + ".avl"),
              record_size(rec_size)
    {
        if (record_size < sizeof(long))
        {
            throw runtime_error("Record size is too small to store the next available pointer.");
        }
        readHead();
    }
    long getNextAvail()
    {
        if (head_position == -1)
        {
            return -1;
        }
        long avail_pos = head_position;

        fstream data_file(data_file_name, ios::in | ios::binary);
        if (!data_file.is_open())
        {
            throw runtime_error("Cannot open data file to read next avail pointer: " + data_file_name);
        }
        data_file.seekg(avail_pos * record_size, ios::beg);

        long next_pos;
        data_file.read(reinterpret_cast<char*>(&next_pos), sizeof(long));
        data_file.close();

        head_position = next_pos;
        writeHead();

        return avail_pos;
    }

    void addAvail(long pos)
    {
        long next_pos = head_position;
        fstream data_file(data_file_name, ios::in | ios::out | ios::binary);
        if (!data_file.is_open())
        {
            throw runtime_error("Cannot open data file to write avail pointer: " + data_file_name);
        }
        data_file.seekp(pos * record_size, ios::beg);
        data_file.write(reinterpret_cast<const char*>(&next_pos), sizeof(long));
        data_file.close();

        head_position = pos;
        writeHead();
    }
};

//constants and structures
const string DOC_DATA_FILE = "doctors.dat";
const int DOC_ID_LEN = 15;
const int DOC_NAME_LEN = 30;
const int DOC_ADDRESS_LEN = 30;

struct DoctorRecord
{
    char doctor_id[DOC_ID_LEN];
    char doctor_name[DOC_NAME_LEN];
    char address[DOC_ADDRESS_LEN];
};


DoctorIndexManager docIndexMgr;
//AvailListManager availListMgr(DOC_DATA_FILE, sizeof(DoctorRecord));


// helper functions
void DoctorWriteFixed(char* dest, const string& s, int size)
{
    memset(dest, 0, size);
    for (int i = 0; i < size && i < (int)s.length(); ++i)
        dest[i] = s[i];
}

string DoctorReadFixed(const char* src, int size)
{
    int len = 0;
    int start_index = (src[0] == '*') ? 1 : 0;
    while (len < size && src[start_index + len] != 0) len++;
    return string(src + start_index, len);
}

bool isRecordDeleted(const DoctorRecord& rec)
{
    return rec.doctor_id[0] == '*';
}

// Function to write a record at a specific position
void writeDoctorRecord(long pos, const DoctorRecord& rec)
{
    fstream file(DOC_DATA_FILE, ios::in | ios::out | ios::binary);
    if (!file.is_open()) throw runtime_error("Cannot open doctors.dat");

    file.seekp(pos * sizeof(DoctorRecord), ios::beg);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(DoctorRecord));
    file.close();
}

// Function to append a record to the end of the file
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


class DoctorManager
{
public:

    bool AddDoctor(const string& id, const string& name, const string& addr)
    {
        if (docIndexMgr.searchByPrimary(id))
        {
            cout << "Doctor ID already exists. Record not added.\n";
            return false;
        }
        DoctorRecord rec;
        DoctorWriteFixed(rec.doctor_id, id, DOC_ID_LEN);
        DoctorWriteFixed(rec.doctor_name, name, DOC_NAME_LEN);
        DoctorWriteFixed(rec.address, addr, DOC_ADDRESS_LEN);
        AvailListManager availListMgr(DOC_DATA_FILE, sizeof(DoctorRecord));
        long pos = -1;
        long avail_pos = availListMgr.getNextAvail();

        if (avail_pos != -1)
        {
            pos = avail_pos;
            writeDoctorRecord(pos, rec);
            cout << "Record written to available slot at position: " << pos << "\n";
        }
        else
        {
            pos = appendDoctorRecord(rec);
            cout << "Record appended to new position: " << pos << "\n";
        }

        int indexPos = docIndexMgr.insertPrimary(id, pos);
        docIndexMgr.insertSecondary(name, indexPos);

        return true;
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
        if (isRecordDeleted(rec))
        {
            cout << "Cannot update deleted doctor.\n";
            return;
        }
        string oldName = DoctorReadFixed(rec.doctor_name, DOC_NAME_LEN);
        docIndexMgr.deleteSecondary(oldName, pos);
        docIndexMgr.insertSecondary(new_name, pos);
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

        if (isRecordDeleted(rec))
        {
            cout << "Doctor already deleted.\n";
            return;
        }
        AvailListManager availListMgr(DOC_DATA_FILE, sizeof(DoctorRecord));
        string doctorName = DoctorReadFixed(rec.doctor_name, DOC_ID_LEN);
        docIndexMgr.deleteSecondary(doctorName, pos);
        rec.doctor_id[0] = '*';
        writeDoctorRecord(pos, rec);
        availListMgr.addAvail(pos);
        cout << "Record at position " << pos << " marked as deleted and added to AVAIL LIST.\n";
        docIndexMgr.deletePrimary(id);
    }


    optional<DoctorRecord> getByDoctorId(const string& id)
    {
        const DocPrimaryIndexEntry* entry = docIndexMgr.searchByPrimary(id);

        if (!entry) return nullopt;
        DoctorRecord rec = readDoctorRecord(entry->offset);
        if (!isRecordDeleted(rec))
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
            if (!isRecordDeleted(rec))
                result.push_back(rec);
        }

        return result;
    }


    static void printRecord(const DoctorRecord& rec)
    {
        cout << "DoctorID: " << DoctorReadFixed(rec.doctor_id, DOC_ID_LEN)
             << " | Name: " << DoctorReadFixed(rec.doctor_name, DOC_NAME_LEN)
             << " | Address: " << DoctorReadFixed(rec.address, DOC_ADDRESS_LEN)
             << "\n";
    }
};