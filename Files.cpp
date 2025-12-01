#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <optional>
#include <filesystem>

#include "IndexManagers.h"

using namespace std;


class AppAvailListManager
{
private:
    const string Appdata_file_name;
    const string Appavail_file_name;
    const size_t Apprecord_size;
    long Apphead_position; // Record number of the first available slot

    // Read the head position from the avail list file
    void AppreadHead()
    {
        ifstream file(Appavail_file_name, ios::binary);
        if (!file.is_open())
        {
            // If file doesn't exist, list is empty
            Apphead_position = -1;
            return;
        }

        file.read(reinterpret_cast<char*>(&Apphead_position), sizeof(long));
        file.close();
    }

    // Write the head position to the avail list file
    void AppwriteHead() const
    {
        ofstream file(Appavail_file_name, ios::binary | ios::trunc);
        if (!file.is_open())
        {
            // This should not happen if the data file can be opened
            throw runtime_error("Cannot open AVAIL LIST file for writing: " + Appavail_file_name);
        }
        file.write(reinterpret_cast<const char*>(&Apphead_position), sizeof(long));
        file.close();
    }

public:
    AppAvailListManager(const string& data_file, size_t rec_size)
            : Appdata_file_name(data_file),
              Appavail_file_name(data_file + ".avl"),
              Apprecord_size(rec_size)
    {
        // Ensure the record is large enough to store the next pointer (a long)
        if (Apprecord_size < sizeof(long))
        {
            throw runtime_error("Record size is too small to store the next available pointer.");
        }
        AppreadHead();
    }

    // Get the next available record position.
    // Returns -1 if the list is empty.
    long AppgetNextAvail()
    {
        if (Apphead_position == -1)
        {
            return -1; // List is empty
        }

        long avail_pos = Apphead_position;

        // The next available position is stored in the first part of the record at head_position
        fstream data_file(Appdata_file_name, ios::in | ios::binary);
        if (!data_file.is_open())
        {
            throw runtime_error("Cannot open data file to read next avail pointer: " + Appdata_file_name);
        }

        // Seek to the beginning of the record
        data_file.seekg(avail_pos * Apprecord_size, ios::beg);

        // Read the next pointer (which is a long)
        long next_pos;
        data_file.read(reinterpret_cast<char*>(&next_pos), sizeof(long));
        data_file.close();

        // Update the head pointer
        Apphead_position = next_pos;
        AppwriteHead();

        return avail_pos;
    }

    // Add a deleted record position to the avail list.
    void AppaddAvail(long pos)
    {
        // 1. Get the current head position (which is the next pointer for the new available slot)
        long next_pos = Apphead_position;

        // 2. Write the old head position (next_pos) into the first part of the record at 'pos'
        fstream data_file(Appdata_file_name, ios::in | ios::out | ios::binary);
        if (!data_file.is_open())
        {
            throw runtime_error("Cannot open data file to write avail pointer: " + Appdata_file_name);
        }

        // Seek to the beginning of the record
        data_file.seekp(pos * Apprecord_size, ios::beg);

        // Write the next pointer (old head)
        data_file.write(reinterpret_cast<const char*>(&next_pos), sizeof(long));
        data_file.close();

        // 3. Update the head pointer to the new available slot 'pos'
        Apphead_position = pos;
        AppwriteHead();
    }
};

// =========================================================================
//                  CONSTANTS AND STRUCTURES
// =========================================================================

const string APPT_DATA_FILE = "appointments.dat";

// Fixed sizes for fields
const int ID_LEN = 15;
const int PID_LEN = 15;
const int DID_LEN = 15;
const int DATE_LEN = 12;
const int TIME_LEN = 8;
// STATUS_LEN and 'status' field are removed.
// The first byte of appointment_id will be used for the deletion marker.

// Struct for Appointment Record
struct AppointmentRecord {
    char appointment_id[ID_LEN];
    char patient_id[PID_LEN];
    char doctor_id[DID_LEN];
    char date[DATE_LEN];
    char time[TIME_LEN];
    // char status[STATUS_LEN]; // REMOVED
};

// =========================================================================
//                  GLOBAL MANAGERS
// =========================================================================

// Global index manager for appointments
AppointmentIndexManager apptIndexMgr;

// Global AVAIL LIST manager (NEW)
// Initialize with the data file name and the size of the record
AppAvailListManager availListMgr(APPT_DATA_FILE, sizeof(AppointmentRecord));


// =========================================================================
//                  HELPER & FILE I/O FUNCTIONS
// =========================================================================

// Generic writeFixed (used for all fields)
void writeFixed(char* dest, const string& s, int size) {
    memset(dest, 0, size);
    for (int i = 0; i < size && i < (int)s.length(); ++i) {
        dest[i] = s[i];
    }
}

// Generic readFixed (used for all fields)
string readFixed(const char* src, int size) {
    int len = 0;
    while (len < size && src[len] != 0) len++;
    return string(src, len);
}

// Specialized readFixed for the primary key (appointment_id) to handle the '*' marker
string readApptIdFixed(const char* src, int size) {
    int len = 0;
    // Start reading from the second byte if the first byte is the deletion marker '*'
    int start_index = (src[0] == '*') ? 1 : 0;

    while (len < size && src[start_index + len] != 0) len++;
    return string(src + start_index, len);
}

// Checks if the record is logically deleted by checking the first byte of the primary key
bool isRecordDeleted(const AppointmentRecord& rec)
{
    return rec.appointment_id[0] == '*';
}

// Data file I/O operations
long appendRecord(const AppointmentRecord& rec) {
    fstream file(APPT_DATA_FILE, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        // Create file if it doesn't exist
        file.open(APPT_DATA_FILE, ios::out | ios::binary);
        file.close();
        file.open(APPT_DATA_FILE, ios::in | ios::out | ios::binary);
    }
    file.seekp(0, ios::end);
    long pos = file.tellp() / sizeof(AppointmentRecord);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(AppointmentRecord));
    file.close();
    return pos;
}

void writeRecord(long pos, const AppointmentRecord& rec) {
    fstream file(APPT_DATA_FILE, ios::in | ios::out | ios::binary);
    if (!file.is_open()) throw runtime_error("Cannot open data file");
    file.seekp(pos * sizeof(AppointmentRecord), ios::beg);
    file.write(reinterpret_cast<const char*>(&rec), sizeof(AppointmentRecord));
    file.close();
}

AppointmentRecord readRecord(long pos) {
    ifstream file(APPT_DATA_FILE, ios::binary);
    if (!file.is_open()) throw runtime_error("Cannot open data file");
    file.seekg(pos * sizeof(AppointmentRecord), ios::beg);
    AppointmentRecord rec;
    if (!file.read(reinterpret_cast<char*>(&rec), sizeof(AppointmentRecord))) {
        throw runtime_error("Failed to read record at position " + to_string(pos));
    }
    file.close();
    return rec;
}

// =========================================================================
//                  APPOINTMENT MANAGER CLASS
// =========================================================================

class AppointmentManager {
public:
    AppointmentManager() {}
    ~AppointmentManager() {}

    void addAppointment(const string& appId, const string& patientId, const string& doctorId,
                        const string& date, const string& time) {
        // o If the record to be added already exits, do not write that record to the file.
        if (apptIndexMgr.searchByPrimary(appId)) {
            cout << "Appointment ID " << appId << " already exists\n";
            return;
        }

        AppointmentRecord rec;
        writeFixed(rec.appointment_id, appId, ID_LEN);
        writeFixed(rec.patient_id, patientId, PID_LEN);
        writeFixed(rec.doctor_id, doctorId, DID_LEN);
        writeFixed(rec.date, date, DATE_LEN);
        writeFixed(rec.time, time, TIME_LEN);
        // Status field removed

        long pos;

        // o When you add a record, first look at the AVAIL LIST, then write the record.
        long avail_pos = availListMgr.AppgetNextAvail();

        if (avail_pos != -1) {
            // o If there is a record available in the AVAIL LIST, write the record to a record AVAIL LIST
            //   points and make appropriate changes on the AVAIL LIST.
            pos = avail_pos;
            writeRecord(pos, rec);
            cout << "Record written to available slot at position: " << pos << "\n";
        }
        else {
            // If AVAIL LIST is empty, append to the end of the file (original logic)
            pos = appendRecord(rec);
            cout << "Record appended to new position: " << pos << "\n";
        }

        int primaryPos = apptIndexMgr.insertPrimary(appId, pos);
        apptIndexMgr.insertSecondary(doctorId, primaryPos);
    }

    void updateAppointmentDate(const string& appId, const string& newDate, const string& newTime) {
        const ApptPrimaryIndexEntry* entry = apptIndexMgr.searchByPrimary(appId);
        if (!entry) {
            cout << "Appointment not found\n";
            return;
        }
        long pos = entry->offset;
        AppointmentRecord rec = readRecord(pos);

        // Check for deletion marker
        if (isRecordDeleted(rec)) {
            cout << "Cannot update deleted appointment\n";
            return;
        }

        writeFixed(rec.date, newDate, DATE_LEN);
        writeFixed(rec.time, newTime, TIME_LEN);
        writeRecord(pos, rec);
    }

    void deleteAppointment(const string& appId) {
        const ApptPrimaryIndexEntry* entry = apptIndexMgr.searchByPrimary(appId);
        if (!entry) {
            cout << "Appointment not found\n";
            return;
        }
        long pos = entry->offset;
        AppointmentRecord rec = readRecord(pos);

        // Check for deletion marker
        if (isRecordDeleted(rec)) {
            cout << "Appointment already deleted\n";
            return;
        }

        // o When you delete a record, do not physically delete the record from file, just put a
        //   marker (*) on the file and make appropriate changes on AVAIL LIST.

        // Put a marker '*' on the first byte of the primary key field
        rec.appointment_id[0] = '*';
        writeRecord(pos, rec);

        // Add the position to the AVAIL LIST
        availListMgr.AppaddAvail(pos);
        cout << "Record at position " << pos << " marked as deleted and added to AVAIL LIST.\n";

        string doctorId = readFixed(rec.doctor_id, DID_LEN);

        int deletedPos = apptIndexMgr.deletePrimary(appId);

        if (deletedPos != -1) {
            apptIndexMgr.deleteSecondary(doctorId, deletedPos);
        }
    }

    vector<AppointmentRecord> getByDoctorId(const string& doctorId) {
        vector<AppointmentRecord> result;

        vector<const ApptPrimaryIndexEntry*> primaryEntries = apptIndexMgr.searchBySecondary(doctorId);

        for (const auto* entry : primaryEntries) {
            try {
                AppointmentRecord rec = readRecord(entry->offset);
                // Check for deletion marker
                if (!isRecordDeleted(rec)) {
                    result.push_back(rec);
                }
            }
            catch (const runtime_error& e) {
            }
        }
        return result;
    }

    optional<AppointmentRecord> getByAppointmentId(const string& appId) {
        const ApptPrimaryIndexEntry* entry = apptIndexMgr.searchByPrimary(appId);

        if (!entry) {
            return nullopt;
        }

        AppointmentRecord rec = readRecord(entry->offset);

        // Check for deletion marker
        if (!isRecordDeleted(rec)) {
            return rec;
        }

        return nullopt;
    }

    static void printRecord(const AppointmentRecord& rec) {
        cout << "AppointmentID: " << readApptIdFixed(rec.appointment_id, ID_LEN)
             << " | PatientID: " << readFixed(rec.patient_id, PID_LEN)
             << " | DoctorID: " << readFixed(rec.doctor_id, DID_LEN)
             << " | Date: " << readFixed(rec.date, DATE_LEN)
             << " | Time: " << readFixed(rec.time, TIME_LEN) << "\n";
    }
};