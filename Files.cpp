#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <optional>
#include "IndexManagers.h"

using namespace std;

// =========================================================================
//                  GLOBAL DEFINITIONS (Fixes Linker Error)
// =========================================================================

// Definition for the global index manager instance
AppointmentIndexManager apptIndexMgr;

long getAppointmentAvailSlot(size_t record_size) {
    return -1;
}

// Mock: Prints a message instead of managing an actual linked list of free slots.
void addAppointmentToAvailList(long offset, size_t record_size) {
    cout << "--- Mock: Added record at disk offset " << offset << " to Avail List ---\n";
}

// =========================================================================
//                  CONSTANTS AND STRUCTURES (From Files.cpp)
// =========================================================================

const string APPT_DATA_FILE = "appointments.dat";

// Fixed sizes for fields
const int ID_LEN = 15;
const int PID_LEN = 15;
const int DID_LEN = 15;
const int DATE_LEN = 12;
const int TIME_LEN = 8;
const int STATUS_LEN = 8;

// Struct for Appointment Record
struct AppointmentRecord {
    char appointment_id[ID_LEN];
    char patient_id[PID_LEN];
    char doctor_id[DID_LEN];
    char date[DATE_LEN];
    char time[TIME_LEN];
    char status[STATUS_LEN];
};

// =========================================================================
//                  HELPER & FILE I/O FUNCTIONS
// =========================================================================

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

// Data file I/O operations
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

        long pos;
        size_t record_size = sizeof(AppointmentRecord);
        pos = getAppointmentAvailSlot(record_size);

        if (pos != -1) {
            writeRecord(pos, rec);
        } else {
            pos = appendRecord(rec);
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

        string status = readFixed(rec.status, STATUS_LEN);
        if (status != "Active") {
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

        string status = readFixed(rec.status, STATUS_LEN);
        if (status == "Deleted") {
            cout << "Appointment already deleted\n";
            return;
        }

        writeFixed(rec.status, "Deleted", STATUS_LEN);
        writeRecord(pos, rec);
        addAppointmentToAvailList(pos, sizeof(AppointmentRecord));

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
                if (readFixed(rec.status, STATUS_LEN) == "Active") {
                    result.push_back(rec);
                }
            } catch (const runtime_error& e) {
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

        if (readFixed(rec.status, STATUS_LEN) == "Active") {
            return rec;
        }

        return nullopt;
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

// =========================================================================
//                  MAIN FUNCTION (With Error Handling)
// =========================================================================

int main() {
    try {
        cout << "--- Hospital Appointment Management System Demonstration ---\n";
        // Instantiate the manager
        AppointmentManager manager;

        // --- 1. Insertion ---
        cout << "\n## 1. Inserting Appointments\n";
        manager.addAppointment("A001", "P001", "D001", "2024-12-01", "10:00");
        manager.addAppointment("A002", "P002", "D002", "2024-12-01", "11:00");
        manager.addAppointment("A003", "P003", "D001", "2024-12-02", "09:00");
        manager.addAppointment("A004", "P004", "D003", "2024-12-02", "15:30");
        manager.addAppointment("A005", "P005", "D002", "2024-12-03", "14:00");
        cout << "5 appointments inserted.\n";

        // Attempt to insert duplicate
        manager.addAppointment("A001", "PXXX", "DXXX", "XXXX-XX-XX", "XX:XX");

        // --- 2. Search by Primary Key (Appointment ID) ---
        cout << "\n## 2. Search by Primary Key (A003)\n";
        auto appt3 = manager.getByAppointmentId("A003");
        if (appt3) {
            cout << "Found A003: ";
            AppointmentManager::printRecord(*appt3);
        } else {
            cout << "Error: A003 not found.\n";
        }

        // Search for non-existent ID
        cout << "\nSearch for non-existent ID (A999):\n";
        if (!manager.getByAppointmentId("A999")) {
            cout << "A999 not found (as expected).\n";
        }


        // --- 3. Search by Secondary Key (Doctor ID) ---
        cout << "\n## 3. Search by Secondary Key (D001)\n";
        vector<AppointmentRecord> doc1_appts = manager.getByDoctorId("D001");
        cout << "Appointments for Doctor D001 (" << doc1_appts.size() << " found):\n";
        for (const auto& rec : doc1_appts) {
            AppointmentManager::printRecord(rec);
        }

        // --- 4. Update an Appointment ---
        cout << "\n## 4. Updating Appointment A004 Date/Time\n";
        manager.updateAppointmentDate("A004", "2024-12-25", "10:30");

        // Verify update
        auto appt4_updated = manager.getByAppointmentId("A004");
        if (appt4_updated) {
            cout << "A004 updated: ";
            AppointmentManager::printRecord(*appt4_updated);
        }

        // --- 5. Delete an Appointment ---
        cout << "\n## 5. Deleting Appointment A002\n";
        manager.deleteAppointment("A002");

        // Try to search for the deleted appointment
        cout << "\nSearch for deleted A002:\n";
        auto deleted_appt = manager.getByAppointmentId("A002");
        if (!deleted_appt) {
            cout << "A002 successfully recognized as logically deleted/not found by getByAppointmentId.\n";
        } else {
            cout << "Error: A002 was found when it shouldn't be.\n";
        }

        // Verify deletion via Secondary Key search (D002 only has A005 left)
        cout << "\nSearch for Doctor D002 appointments after A002 deletion:\n";
        vector<AppointmentRecord> doc2_appts = manager.getByDoctorId("D002");
        cout << "Appointments for Doctor D002 (" << doc2_appts.size() << " found):\n";
        for (const auto& rec : doc2_appts) {
            AppointmentManager::printRecord(rec);
        }
        cout << "\n--- Demonstration complete. Index data saved to files ---\n";
    } catch (const std::runtime_error& e) {
        // CATCHING THE FILE I/O ERROR HERE
        cerr << "\nFATAL RUNTIME ERROR CAUGHT: " << e.what() << endl;
        return 3;
    } catch (const std::exception& e) {
        cerr << "\nUNEXPECTED EXCEPTION CAUGHT: " << e.what() << endl;
        return 4;
    } catch (...) {
        cerr << "\nUNKNOWN ERROR CAUGHT. Program terminating." << endl;
        return 5;
    }
    return 0;
}