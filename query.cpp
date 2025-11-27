#include "Files.cpp"
#include "parser.cpp"
#include <sstream>

class QueryManger {
private:
  Parser parser;

  string buildRecordString(const AppointmentRecord &rec) {
    stringstream ss;
    bool first = true;

    for (const auto &field : parser.selectFields) {
      if (!first)
        ss << ", ";
      first = false;

      if (field == "all") {
        ss << "Appointment ID: " << readFixed(rec.appointment_id, ID_LEN)
           << ", Patient ID: " << readFixed(rec.patient_id, PID_LEN)
           << ", Doctor ID: " << readFixed(rec.doctor_id, DID_LEN)
           << ", Date: " << readFixed(rec.date, DATE_LEN)
           << ", Time: " << readFixed(rec.time, TIME_LEN)
           << ", Status: " << readFixed(rec.status, STATUS_LEN);
        break;
      } else if (field == "appointment_id") {
        ss << "Appointment ID: " << readFixed(rec.appointment_id, ID_LEN);
      } else if (field == "patient_id") {
        ss << "Patient ID: " << readFixed(rec.patient_id, PID_LEN);
      } else if (field == "doctor_id") {
        ss << "Doctor ID: " << readFixed(rec.doctor_id, DID_LEN);
      } else if (field == "date") {
        ss << "Date: " << readFixed(rec.date, DATE_LEN);
      } else if (field == "time") {
        ss << "Time: " << readFixed(rec.time, TIME_LEN);
      } else if (field == "status") {
        ss << "Status: " << readFixed(rec.status, STATUS_LEN);
      }
    }

    return ss.str();
  }

  void handleAppointmentsTable() {
    AppointmentManager apptMgr = AppointmentManager();

    if (parser.columnName == "doctor_id") {
      vector<AppointmentRecord> records =
          apptMgr.getByDoctorId(parser.columnValue);
      for (const auto &rec : records) {
        cout << buildRecordString(rec) << endl;
      }
    } else if (parser.columnName == "appointment_id") {
      optional<AppointmentRecord> recOpt =
          apptMgr.getByAppointmentId(parser.columnValue);
      if (recOpt.has_value()) {
        cout << buildRecordString(recOpt.value()) << endl;
      } else {
        cout << "No active record found for Appointment ID: "
             << parser.columnValue << endl;
      }
    } else {
      cout << "Unsupported WHERE column: " << parser.columnName << endl;
    }
  }

  void handleTableQuery(const string &tableName) {
    if (tableName == "appointments") {
      handleAppointmentsTable();
    } else {
      cout << "Unsupported table: " << tableName << endl;
    }
  }

public:
  QueryManger() { this->parser = Parser(); }
  void makeQuery(string query) {
    try {
      parser.parse(query);
    } catch (const invalid_argument &e) {
      cout << "Query Error: " << e.what() << endl;
      return;
    }

    string tableName = parser.tableName;
    handleTableQuery(tableName);
  }
};

/*int main() {
  // Add test data first
  AppointmentManager mgr;
  mgr.addAppointment("A001", "P001", "D001", "2024-01-15", "10:00");
  mgr.addAppointment("A002", "P002", "D001", "2024-01-16", "11:00");
  mgr.addAppointment("A003", "P003", "D002", "2024-01-17", "14:00");

  QueryManger qm;

  cout << "=== Test Case 1: Select all fields by doctor_id ===" << endl;
  qm.makeQuery("SELECT all FROM appointments WHERE doctor_id = D001");
  cout << endl;

  cout << "=== Test Case 2: Select specific fields by doctor_id ===" << endl;
  qm.makeQuery("SELECT appointment_id, date, time FROM appointments WHERE "
               "doctor_id = D001");
  cout << endl;

  cout << "=== Test Case 3: Select single field by appointment_id ===" << endl;
  qm.makeQuery("SELECT status FROM appointments WHERE appointment_id = A001");
  cout << endl;

  cout << "=== Test Case 4: Select multiple fields ===" << endl;
  qm.makeQuery("SELECT patient_id, doctor_id, status FROM appointments WHERE "
               "appointment_id = A001");
  cout << endl;

  cout << "=== Test Case 5: Non-existent appointment_id ===" << endl;
  qm.makeQuery("SELECT all FROM appointments WHERE appointment_id = A999");
  cout << endl;

  return 0;
}*/
