#include <iostream>
#include <string>
#include <limits>
#include "query.cpp"

using namespace std;

DoctorManager docMgr;

int main() {


    bool running = true;
    while (running) {
        cout << "\nMain Menu\n";
        cout << "1. Add New Doctor\n";
        cout << "2. Add New Appointment\n";
        cout << "3. Update Doctor Name (Doctor ID)\n";
        cout << "4. Update Appointment Date (Appointment ID)\n";
        cout << "5. Delete Appointment (Appointment ID)\n";
        cout << "6. Delete Doctor (Doctor ID)\n";
        cout << "7. Print Doctor Info (Doctor ID)\n";
        cout << "8. Print Appointment Info (Appointment ID)\n";
        cout << "9. Write Query\n";
        cout << "10. Exit\n";
        cout << "Choose an option: ";

        int choice = 0;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1: {
                string id, name, address;
    
                cout << "Enter Doctor ID: ";
                getline(cin, id);
    
                cout << "Enter Doctor Name: ";
                getline(cin, name);
    
                cout << "Enter Doctor Address: ";
                getline(cin, address);
    
                if(docMgr.AddDoctor(id, name, address)) {
                     cout << "Doctor added.\n";
                }
                break;
            }
            case 2: {
                string appointmentID, patientID, doctorID, date, appointmentTime;
                
                cout << "Enter Appointment ID: ";
                getline(cin, appointmentID);
                
                cout << "Enter Patient ID: ";
                getline(cin, patientID);
                
                cout << "Enter Doctor ID: ";
                getline(cin, doctorID);
                
                cout << "Enter Appointment Date (YYYY-MM-DD): ";
                getline(cin, date);
                
                cout << "Enter Appointment Time (HH:MM): ";
                getline(cin, appointmentTime);
                
                AppointmentManager manager;
                manager.addAppointment(appointmentID, patientID, doctorID, date, appointmentTime);
                cout << "Appointment added successfully.\n";
                break;
            }
            case 3: {
                string id, new_name;
    
                cout << "Enter Doctor ID to update: ";
                getline(cin, id);
    
                cout << "Enter new Doctor Name: ";
                getline(cin, new_name);
    
                docMgr.UpdateDoctorName(id, new_name);
                break;
            }
            case 4: {
                string appointmentID, newDate, newTime;
                
                cout << "Enter Appointment ID: ";
                getline(cin, appointmentID);
                
                cout << "Enter New Appointment Date (YYYY-MM-DD): ";
                getline(cin, newDate);
                
                cout << "Enter New Appointment Time (HH:MM): ";
                getline(cin, newTime);
                
                AppointmentManager manager;
                manager.updateAppointmentDate(appointmentID, newDate, newTime);
                cout << "Appointment updated successfully.\n";
                break;
            }
            case 5: {
                string appointmentID;
                
                cout << "Enter Appointment ID to delete: ";
                getline(cin, appointmentID);
                
                AppointmentManager manager;
                manager.deleteAppointment(appointmentID);
                cout << "Appointment deleted successfully.\n";
                break;
            }
            case 6: {
                string id;
    
                cout << "Enter Doctor ID to delete: ";
                getline(cin, id);
    
                docMgr.DeleteDoctor(id);
                break;
            }
            case 7: {
                string id;
    
                cout << "Enter Doctor ID: ";
                getline(cin, id);
    
                auto doc = docMgr.getByDoctorId(id);

                if (doc)
                    DoctorManager::printRecord(*doc);
                else
                    cout << "Doctor not found.\n";

                break;
            }
            case 8: {
                string appointmentID;
                
                cout << "Enter Appointment ID: ";
                getline(cin, appointmentID);
                
                AppointmentManager manager;
                auto appt = manager.getByAppointmentId(appointmentID);
                if (appt) {
                    cout << "Found " << appointmentID << ": ";
                    AppointmentManager::printRecord(*appt);
                } else {
                    cout << "Error: " << appointmentID << " not found.\n";
                }

                break;
            }
            case 9: {
                QueryManger qm;
                string query;
                cout << "Enter query: ";
                getline(cin, query); 
                qm.makeQuery(query);
                cout << endl;

                break;
            }
            case 10: {
                running = false;
                break;
            }
            default: {
                cout << "Unknown option. Please choose a number from 1 to 10.\n";
                break;
            }
        }
    }

    cout << "Exiting.\n";
    return 0;
}
