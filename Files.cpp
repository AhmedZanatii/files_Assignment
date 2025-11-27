#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <optional>
#include "IndexManagers.h" // Assuming this is Ahmed's file

using namespace std;

// --- EXTERNAL DEPENDENCIES ---
// The global index manager instance (provided by Ahmed's setup)
extern AppointmentIndexManager apptIndexMgr;
// The Avail List functions (must be implemented by the team, using a shared list structure)
extern long getAppointmentAvailSlot(size_t record_size);
extern void addAppointmentToAvailList(long offset, size_t record_size);


// File names
const string APPT_DATA_FILE = "appointments.dat";
// The following constants are now *only* relevant for Dodz's old code
// or will be ignored, but we redefine APPT_DATA_FILE to avoid conflicts.

// Fixed sizes for fields
const int ID_LEN = 16;
const int PID_LEN = 24;
const int DID_LEN = 16;
const int DATE_LEN = 12;
const int TIME_LEN = 8;
const int STATUS_LEN = 8;

// Struct for Appointment Record (remains unchanged)
struct AppointmentRecord {
    char appointment_id[ID_LEN];
    char patient_id[PID_LEN];
    char doctor_id[DID_LEN];
    char date[DATE_LEN];
    char time[TIME_LEN];
    char status[STATUS_LEN];
};

// Helper functions (remain unchanged)
void writeFixed(char* dest, const string& s, int size) {
    memset(dest, 0, size);
    for (int i = 0; i < size && i < (int)s.length(); ++i) {
        dest[i] = s[i];
    }
}
string readFixed(const char* src, int size) {
    int len = 0;
    while (len < size && src[len] != 0) len++;
    return string(src, len);
}

// Data file I/O operations (remain unchanged)
long appendRecord(const AppointmentRecord& rec) {
    fstream file(APPT_DATA_FILE, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
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
    file.read(reinterpret_cast<char*>(&rec), sizeof(AppointmentRecord));
    file.close();
    return rec;
}


class AppointmentManager {
private:
    // *** REMOVED: primaryIndex, secondaryIndex, availList ***
    // *** REMOVED: loadIndexes() and saveIndexes() ***

public:
    AppointmentManager() {}
    ~AppointmentManager() {} // Ahmed's index manager destructor handles saving indexes.

    void addAppointment(const string& appId, const string& patientId, const string& doctorId,
                        const string& date, const string& time) {
        // 1. Check Index using Ahmed's binary search
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
        writeFixed(rec.status, "Active", STATUS_LEN);

        // 2. Write record to file (using Avail List or Append logic)
        long pos;
        size_t record_size = sizeof(AppointmentRecord);
        pos = getAppointmentAvailSlot(record_size);

        if (pos != -1) {
            writeRecord(pos, rec);
        } else {
            pos = appendRecord(rec);
        }

        // 3. Update Ahmed's Indexes
        // insertPrimary returns the vector position (the binding)
        int primaryPos = apptIndexMgr.insertPrimary(appId, pos);
        // secondary key is doctorId, linked via primaryPos
        apptIndexMgr.insertSecondary(doctorId, primaryPos);
    }

    void updateAppointmentDate(const string& appId, const string& newDate, const string& newTime) {
        // 1. Search Index to get disk offset
        const ApptPrimaryIndexEntry* entry = apptIndexMgr.searchByPrimary(appId);
        if (!entry) {
            cout << "Appointment not found\n";
            return;
        }
        long pos = entry->offset;
        AppointmentRecord rec = readRecord(pos);

        string status = readFixed(rec.status, STATUS_LEN);
        if (status != "Active") {
            cout << "Cannot update deleted appointment\n";
            return;
        }

        // 2. Update record on disk
        writeFixed(rec.date, newDate, DATE_LEN);
        writeFixed(rec.time, newTime, TIME_LEN);
        writeRecord(pos, rec);
    }

    void deleteAppointment(const string& appId) {
        // 1. Search Index to get disk offset
        const ApptPrimaryIndexEntry* entry = apptIndexMgr.searchByPrimary(appId);
        if (!entry) {
            cout << "Appointment not found\n";
            return;
        }
        long pos = entry->offset;
        AppointmentRecord rec = readRecord(pos);

        string status = readFixed(rec.status, STATUS_LEN);
        if (status == "Deleted") {
            cout << "Appointment already deleted\n";
            return;
        }

        // 2. Logical Delete on Disk & Add to Avail List
        writeFixed(rec.status, "Deleted", STATUS_LEN);
        writeRecord(pos, rec);
        addAppointmentToAvailList(pos, sizeof(AppointmentRecord));

        // 3. Update Ahmed's Indexes
        string doctorId = readFixed(rec.doctor_id, DID_LEN);

        // deletePrimary removes the entry and returns the position that was deleted
        int deletedPos = apptIndexMgr.deletePrimary(appId);

        // Remove secondary index entry using the secondary key (doctorId)
        // and the position before the primary index shifted.
        if (deletedPos != -1) {
            apptIndexMgr.deleteSecondary(doctorId, deletedPos);
        }
    }

    vector<AppointmentRecord> getByDoctorId(const string& doctorId) {
        vector<AppointmentRecord> result;

        // 1. Use Ahmed's searchBySecondary to get all primary index entries
        vector<const ApptPrimaryIndexEntry*> primaryEntries = apptIndexMgr.searchBySecondary(doctorId);

        // 2. Read records using the offsets provided by Ahmed's index
        for (const auto* entry : primaryEntries) {
            try {
                // entry->offset holds the record position (which Dodz's appendRecord returned)
                AppointmentRecord rec = readRecord(entry->offset);
                // Only return active records
                if (readFixed(rec.status, STATUS_LEN) == "Active") {
                    result.push_back(rec);
                }
            } catch (const runtime_error& e) {
                // Ignore records that can't be read
            }
        }
        return result;
    }

    optional<AppointmentRecord> getByAppointmentId(const string& appId) {
        // 1. Use Ahmed's searchByPrimary (binary search)
        const ApptPrimaryIndexEntry* entry = apptIndexMgr.searchByPrimary(appId);

        if (!entry) {
            return nullopt; // Appointment ID not found in Ahmed's index
        }

        // 2. Read record using the offset
        AppointmentRecord rec = readRecord(entry->offset);

        // Return only if the record is logically "Active"
        if (readFixed(rec.status, STATUS_LEN) == "Active") {
            return rec;
        }

        return nullopt; // Logically deleted record
    }

    static void printRecord(const AppointmentRecord& rec) {
        cout << "AppointmentID: " << readFixed(rec.appointment_id, ID_LEN)
             << " | PatientID: " << readFixed(rec.patient_id, PID_LEN)
             << " | DoctorID: " << readFixed(rec.doctor_id, DID_LEN)
             << " | Date: " << readFixed(rec.date, DATE_LEN)
             << " | Time: " << readFixed(rec.time, TIME_LEN)
             << " | Status: " << readFixed(rec.status, STATUS_LEN) << "\n";
    }
};
