#include "Files.cpp"
#include "parser.cpp"

class QueryManger {
private:
  Parser parser;
  void handleAppointmentsTable() {
    AppointmentManager apptMgr = AppointmentManager();
    if (parser.selectFields.size() == 1 && parser.selectFields[0] == "all") {
      if (parser.columnName == "doctor_id") {
        vector<AppointmentRecord> records =
            apptMgr.getByDoctorId(parser.columnValue);
        for (const auto &rec : records) {
          AppointmentManager::printRecord(rec);
        }
      } else if (parser.columnName == "appointment_id") {
        optional<AppointmentRecord> recOpt =
            apptMgr.getByAppointmentId(parser.columnValue);
        if (recOpt.has_value()) {
          AppointmentManager::printRecord(recOpt.value());
        } else {
          cout << "No active record found for Appointment ID: "
               << parser.columnValue << endl;
        }
      } else {
        cout << "Unsupported WHERE column: " << parser.columnName << endl;
      }
    } else {
      cout << "Only SELECT all is supported in this version." << endl;
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

int main() {
  QueryManger qm;

  cout << "=== Test Case 1: Query appointments by doctor_id ===" << endl;
  qm.makeQuery("SELECT all FROM appointments WHERE doctor_id = D001");
  cout << endl;

  cout << "=== Test Case 2: Query appointments by appointment_id ===" << endl;
  qm.makeQuery("SELECT all FROM appointments WHERE appointment_id = A001");
  cout << endl;

  cout << "=== Test Case 3: Non-existent appointment_id ===" << endl;
  qm.makeQuery("SELECT all FROM appointments WHERE appointment_id = A999");
  cout << endl;

  cout << "=== Test Case 4: Unsupported table ===" << endl;
  qm.makeQuery("SELECT all FROM patients WHERE patient_id = P001");
  cout << endl;

  return 0;
}
