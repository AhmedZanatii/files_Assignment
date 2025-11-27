#ifndef INDEX_MANAGERS_H
#define INDEX_MANAGERS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iterator>
#include <cstdio> // For remove()

using namespace std;

// =========================================================================
//                  I. FILE CONSTANTS & CORE STRUCTURES
// =========================================================================

// Appointment Constants
const string APPT_PRIMARY_INDEX_FILE = "primary.idx";
const string APPT_SECONDARY_INDEX_FILE = "secondary.idx";

// Doctor Constants
const string DOC_PRIMARY_INDEX_FILE = "doctor_primary.idx";
const string DOC_SECONDARY_INDEX_FILE = "doctor_secondary.idx";


// --- Appointment Index Structures ---

struct ApptPrimaryIndexEntry {
    string appointmentId; // Primary Key
    long offset;          // File offset (record position)

    bool operator<(const ApptPrimaryIndexEntry& other) const {
        return appointmentId < other.appointmentId;
    }
};

struct ApptSecondaryIndexNode {
    string doctorId; // Secondary Key
    int primaryIndexPos; // Index into the primaryIndex vector
    int next;            // Index of the next node in the secondary index list

    ApptSecondaryIndexNode(const string& id, int pos)
        : doctorId(id), primaryIndexPos(pos), next(-1) {}
};

// --- Doctor Index Structures ---

struct DocPrimaryIndexEntry {
    string doctorId;
    short offset; // File offset in bytes

    bool operator<(const DocPrimaryIndexEntry& other) const {
        return doctorId < other.doctorId;
    }
};

struct DocSecondaryIndexNode {
    string doctorName; // Secondary Key
    int primaryIndexPos; // Index into the primaryIndex vector
    int next;            // Index of the next node in the secondary index list

    DocSecondaryIndexNode(const string& name, int pos)
        : doctorName(name), primaryIndexPos(pos), next(-1) {}
};

// =========================================================================
//                             APPOINTMENT MANAGER
// =========================================================================

class AppointmentIndexManager {
private:
    vector<ApptPrimaryIndexEntry> primaryIndex;
    vector<ApptSecondaryIndexNode> secondaryIndex;
    unordered_map<string, int> secondaryIndexHeads;

    // Adjusts primaryIndexPos pointers in secondary index after erasure.
    void updateSecondaryIndicesOnErase(int deletedPos) {
        for (auto& node : secondaryIndex) {
            if (node.primaryIndexPos > deletedPos) {
                node.primaryIndexPos--;
            }
        }
    }

    void loadIndexes() {
        // Load Primary Index
        ifstream pIn(APPT_PRIMARY_INDEX_FILE, ios::binary);
        if (pIn.good()) {
            size_t sz;
            pIn.read(reinterpret_cast<char*>(&sz), sizeof(sz));
            primaryIndex.reserve(sz);
            for (size_t i = 0; i < sz; i++) {
                size_t len;
                pIn.read(reinterpret_cast<char*>(&len), sizeof(len));
                string key(len, '\0');
                pIn.read(&key[0], len);
                long offset;
                pIn.read(reinterpret_cast<char*>(&offset), sizeof(offset));
                primaryIndex.push_back({key, offset});
            }
        }

        // Load Secondary Index (Heads and Nodes)
        ifstream sIn(APPT_SECONDARY_INDEX_FILE, ios::binary);
        if (sIn.good()) {
            size_t headsCount;
            sIn.read(reinterpret_cast<char*>(&headsCount), sizeof(headsCount));
            for (size_t i = 0; i < headsCount; i++) {
                size_t len;
                sIn.read(reinterpret_cast<char*>(&len), sizeof(len));
                string key(len, '\0');
                sIn.read(&key[0], len);
                int head;
                sIn.read(reinterpret_cast<char*>(&head), sizeof(head));
                secondaryIndexHeads[key] = head;
            }
            size_t nodesCount;
            sIn.read(reinterpret_cast<char*>(&nodesCount), sizeof(nodesCount));
            secondaryIndex.reserve(nodesCount);
            for (size_t i = 0; i < nodesCount; i++) {
                size_t len;
                sIn.read(reinterpret_cast<char*>(&len), sizeof(len));
                string id(len, '\0');
                sIn.read(&id[0], len);
                int pos, next;
                sIn.read(reinterpret_cast<char*>(&pos), sizeof(pos));
                sIn.read(reinterpret_cast<char*>(&next), sizeof(next));
                ApptSecondaryIndexNode newNode(id, pos);
                newNode.next = next;
                secondaryIndex.push_back(std::move(newNode));
            }
        }
    }

    void saveIndexes() {
        // Save Primary Index
        ofstream pOut(APPT_PRIMARY_INDEX_FILE, ios::binary | ios::trunc);
        if (pOut.good()) {
            size_t sz = primaryIndex.size();
            pOut.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (const auto& p : primaryIndex) {
                size_t len = p.appointmentId.size();
                pOut.write(reinterpret_cast<const char*>(&len), sizeof(len));
                pOut.write(p.appointmentId.c_str(), len);
                pOut.write(reinterpret_cast<const char*>(const_cast<long*>(&p.offset)), sizeof(p.offset));
            }
        } else {
             // Handle error if file can't be opened/written
        }

        // Save Secondary Index Heads and Nodes
        ofstream sOut(APPT_SECONDARY_INDEX_FILE, ios::binary | ios::trunc);
        if (sOut.good()) {
            size_t sz = secondaryIndexHeads.size();
            sOut.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (const auto& entry : secondaryIndexHeads) {
                size_t len = entry.first.size();
                sOut.write(reinterpret_cast<const char*>(&len), sizeof(len));
                sOut.write(entry.first.c_str(), len);
                sOut.write(reinterpret_cast<const char*>(const_cast<int*>(&entry.second)), sizeof(entry.second));
            }

            sz = secondaryIndex.size();
            sOut.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (const auto& node : secondaryIndex) {
                size_t len = node.doctorId.size();
                sOut.write(reinterpret_cast<const char*>(&len), sizeof(len));
                sOut.write(node.doctorId.c_str(), len);
                sOut.write(reinterpret_cast<const char*>(const_cast<int*>(&node.primaryIndexPos)), sizeof(node.primaryIndexPos));
                sOut.write(reinterpret_cast<const char*>(const_cast<int*>(&node.next)), sizeof(node.next));
            }
        }
    }


public:
    AppointmentIndexManager() { loadIndexes(); }
    ~AppointmentIndexManager() { saveIndexes(); }

    // Task 1: Binary Search Implementation
    // Returns index in primaryIndex vector, or -1 if not found.
    int binarySearchPrimary(const string& appointmentId) const {
        int left = 0;
        int right = (int)primaryIndex.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (primaryIndex[mid].appointmentId == appointmentId) return mid;
            else if (primaryIndex[mid].appointmentId < appointmentId) left = mid + 1;
            else right = mid - 1;
        }
        return -1;
    }

    // Task 3: Keeps Primary Index Sorted & Task 4: The Binding
    // Inserts an entry and returns its new position.
    int insertPrimary(const string& appointmentId, long offset) {
        ApptPrimaryIndexEntry newEntry{appointmentId, offset};
        auto it = std::lower_bound(primaryIndex.begin(), primaryIndex.end(), newEntry);
        int pos = std::distance(primaryIndex.begin(), it);
        primaryIndex.insert(it, newEntry);
        return pos;
    }

    // Task 3: Updates Primary Index on delete
    // Deletes an entry and returns its old position for secondary index maintenance.
    int deletePrimary(const string& appointmentId) {
        ApptPrimaryIndexEntry tempEntry{appointmentId, 0};
        auto it = std::lower_bound(primaryIndex.begin(), primaryIndex.end(), tempEntry);

        if (it != primaryIndex.end() && it->appointmentId == appointmentId) {
            int deletedPos = std::distance(primaryIndex.begin(), it);
            primaryIndex.erase(it);

            // Crucial step: Update secondary index pointers
            updateSecondaryIndicesOnErase(deletedPos);
            return deletedPos;
        }
        return -1; // Not found
    }

    // Task 2: Linked-List Secondary Index Management
    // Inserts a new node and binds it to the primary index position.
    void insertSecondary(const string& doctorId, int primaryIndexPos) {
        ApptSecondaryIndexNode newNode(doctorId, primaryIndexPos);
        int newNodeIdx = (int)secondaryIndex.size();
        secondaryIndex.push_back(newNode);

        auto head_it = secondaryIndexHeads.find(doctorId);
        if (head_it == secondaryIndexHeads.end()) {
            secondaryIndexHeads[doctorId] = newNodeIdx;
        } else {
            // Prepend: New node points to old head
            secondaryIndex[newNodeIdx].next = head_it->second;
            // New node becomes the new head
            head_it->second = newNodeIdx;
        }
    }

    // Task 2/3: Linked-List Secondary Index Deletion
    void deleteSecondary(const string& doctorId, int primaryIndexPos) {
        auto head_it = secondaryIndexHeads.find(doctorId);
        if (head_it == secondaryIndexHeads.end()) return;

        int currentIdx = head_it->second;
        int prevIdx = -1;

        while (currentIdx != -1) {
            if (secondaryIndex[currentIdx].primaryIndexPos == primaryIndexPos) {
                if (prevIdx == -1) {
                    // Deleted node is the head
                    head_it->second = secondaryIndex[currentIdx].next;
                    if (head_it->second == -1) secondaryIndexHeads.erase(head_it);
                } else {
                    // Deleted node is in the middle/end
                    secondaryIndex[prevIdx].next = secondaryIndex[currentIdx].next;
                }
                // Node is logically unlinked; physical removal from vector is a separate defragmentation task.
                break;
            }
            prevIdx = currentIdx;
            currentIdx = secondaryIndex[currentIdx].next;
        }
    }

    // --- Access/Search Methods ---
    // Retrieves a single primary index entry by primary key
    const ApptPrimaryIndexEntry* searchByPrimary(const string& appointmentId) const {
        int pos = binarySearchPrimary(appointmentId);
        if (pos != -1) return &primaryIndex[pos];
        return nullptr;
    }

    // Retrieves all primary index entries for a given secondary key
    vector<const ApptPrimaryIndexEntry*> searchBySecondary(const string& doctorId) const {
        vector<const ApptPrimaryIndexEntry*> results;
        auto head_it = secondaryIndexHeads.find(doctorId);
        if (head_it == secondaryIndexHeads.end()) return results;

        int currentIdx = head_it->second;
        while (currentIdx != -1) {
            int primaryPos = secondaryIndex[currentIdx].primaryIndexPos;
            results.push_back(&primaryIndex[primaryPos]);
            currentIdx = secondaryIndex[currentIdx].next;
        }
        return results;
    }
};

// =========================================================================
//                               DOCTOR MANAGER
// =========================================================================

class DoctorIndexManager {
private:
    vector<DocPrimaryIndexEntry> primaryIndex;
    vector<DocSecondaryIndexNode> secondaryIndex;
    unordered_map<string, int> secondaryIndexHeads;

    // Adjusts primaryIndexPos pointers in secondary index after erasure.
    void updateSecondaryIndicesOnErase(int deletedPos) {
        for (auto& node : secondaryIndex) {
            if (node.primaryIndexPos > deletedPos) {
                node.primaryIndexPos--;
            }
        }
    }

    void loadIndexes() {
        // Load Primary Index (using short offset)
        ifstream pIn(DOC_PRIMARY_INDEX_FILE, ios::binary);
        if (pIn.good()) {
            size_t sz;
            pIn.read(reinterpret_cast<char*>(&sz), sizeof(sz));
            primaryIndex.reserve(sz);
            for (size_t i = 0; i < sz; i++) {
                size_t len;
                pIn.read(reinterpret_cast<char*>(&len), sizeof(len));
                string key(len, '\0');
                pIn.read(&key[0], len);
                short offset;
                pIn.read(reinterpret_cast<char*>(&offset), sizeof(offset));
                primaryIndex.push_back({key, offset});
            }
        }

        // Load Secondary Index (Heads and Nodes)
        ifstream sIn(DOC_SECONDARY_INDEX_FILE, ios::binary);
        if (sIn.good()) {
            size_t headsCount;
            sIn.read(reinterpret_cast<char*>(&headsCount), sizeof(headsCount));
            for (size_t i = 0; i < headsCount; i++) {
                size_t len;
                sIn.read(reinterpret_cast<char*>(&len), sizeof(len));
                string key(len, '\0');
                sIn.read(&key[0], len);
                int head;
                sIn.read(reinterpret_cast<char*>(&head), sizeof(head));
                secondaryIndexHeads[key] = head;
            }
            size_t nodesCount;
            sIn.read(reinterpret_cast<char*>(&nodesCount), sizeof(nodesCount));
            secondaryIndex.reserve(nodesCount);
            for (size_t i = 0; i < nodesCount; i++) {
                size_t len;
                sIn.read(reinterpret_cast<char*>(&len), sizeof(len));
                string name(len, '\0');
                sIn.read(&name[0], len);
                int pos, next;
                sIn.read(reinterpret_cast<char*>(&pos), sizeof(pos));
                sIn.read(reinterpret_cast<char*>(&next), sizeof(next));
                DocSecondaryIndexNode newNode(name, pos);
                newNode.next = next;
                secondaryIndex.push_back(std::move(newNode));
            }
        }
    }

    void saveIndexes() {
        // Save Primary Index
        ofstream pOut(DOC_PRIMARY_INDEX_FILE, ios::binary | ios::trunc);
        if (pOut.good()) {
            size_t sz = primaryIndex.size();
            pOut.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (const auto& p : primaryIndex) {
                size_t len = p.doctorId.size();
                pOut.write(reinterpret_cast<const char*>(&len), sizeof(len));
                pOut.write(p.doctorId.c_str(), len);
                pOut.write(reinterpret_cast<const char*>(const_cast<short*>(&p.offset)), sizeof(p.offset));
            }
        }

        // Save Secondary Index Heads and Nodes
        ofstream sOut(DOC_SECONDARY_INDEX_FILE, ios::binary | ios::trunc);
        if (sOut.good()) {
            size_t sz = secondaryIndexHeads.size();
            sOut.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (const auto& entry : secondaryIndexHeads) {
                size_t len = entry.first.size();
                sOut.write(reinterpret_cast<const char*>(&len), sizeof(len));
                sOut.write(entry.first.c_str(), len);
                sOut.write(reinterpret_cast<const char*>(const_cast<int*>(&entry.second)), sizeof(entry.second));
            }

            sz = secondaryIndex.size();
            sOut.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            for (const auto& node : secondaryIndex) {
                size_t len = node.doctorName.size();
                sOut.write(reinterpret_cast<const char*>(&len), sizeof(len));
                sOut.write(node.doctorName.c_str(), len);
                sOut.write(reinterpret_cast<const char*>(const_cast<int*>(&node.primaryIndexPos)), sizeof(node.primaryIndexPos));
                sOut.write(reinterpret_cast<const char*>(const_cast<int*>(&node.next)), sizeof(node.next));
            }
        }
    }


public:
    DoctorIndexManager() { loadIndexes(); }
    ~DoctorIndexManager() { saveIndexes(); }

    // Task 1: Binary Search Implementation
    int binarySearchPrimary(const string& doctorId) const {
        int left = 0;
        int right = (int)primaryIndex.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (primaryIndex[mid].doctorId == doctorId) return mid;
            else if (primaryIndex[mid].doctorId < doctorId) left = mid + 1;
            else right = mid - 1;
        }
        return -1;
    }

    // Task 3: Keeps Primary Index Sorted & Task 4: The Binding
    int insertPrimary(const string& doctorId, short offset) {
        DocPrimaryIndexEntry newEntry{doctorId, offset};
        auto it = std::lower_bound(primaryIndex.begin(), primaryIndex.end(), newEntry);
        int pos = std::distance(primaryIndex.begin(), it);
        primaryIndex.insert(it, newEntry);
        return pos;
    }

    // Task 3: Updates Primary Index on delete
    int deletePrimary(const string& doctorId) {
        DocPrimaryIndexEntry tempEntry{doctorId, 0};
        auto it = std::lower_bound(primaryIndex.begin(), primaryIndex.end(), tempEntry);

        if (it != primaryIndex.end() && it->doctorId == doctorId) {
            int deletedPos = std::distance(primaryIndex.begin(), it);
            primaryIndex.erase(it);
            updateSecondaryIndicesOnErase(deletedPos);
            return deletedPos;
        }
        return -1;
    }

    // Task 2: Linked-List Secondary Index Management
    void insertSecondary(const string& doctorName, int primaryIndexPos) {
        DocSecondaryIndexNode newNode(doctorName, primaryIndexPos);
        int newNodeIdx = (int)secondaryIndex.size();
        secondaryIndex.push_back(newNode);

        auto head_it = secondaryIndexHeads.find(doctorName);
        if (head_it == secondaryIndexHeads.end()) {
            secondaryIndexHeads[doctorName] = newNodeIdx;
        } else {
            // Prepend: New node points to old head
            secondaryIndex[newNodeIdx].next = head_it->second;
            // New node becomes the new head
            head_it->second = newNodeIdx;
        }
    }

    // Task 2/3: Linked-List Secondary Index Deletion
    void deleteSecondary(const string& doctorName, int primaryIndexPos) {
        auto head_it = secondaryIndexHeads.find(doctorName);
        if (head_it == secondaryIndexHeads.end()) return;

        int currentIdx = head_it->second;
        int prevIdx = -1;

        while (currentIdx != -1) {
            if (secondaryIndex[currentIdx].primaryIndexPos == primaryIndexPos) {
                if (prevIdx == -1) {
                    head_it->second = secondaryIndex[currentIdx].next;
                    if (head_it->second == -1) secondaryIndexHeads.erase(head_it);
                } else {
                    secondaryIndex[prevIdx].next = secondaryIndex[currentIdx].next;
                }
                break;
            }
            prevIdx = currentIdx;
            currentIdx = secondaryIndex[currentIdx].next;
        }
    }

    // --- Access/Search Methods ---
    const DocPrimaryIndexEntry* searchByPrimary(const string& doctorId) const {
        int pos = binarySearchPrimary(doctorId);
        if (pos != -1) return &primaryIndex[pos];
        return nullptr;
    }

    vector<const DocPrimaryIndexEntry*> searchBySecondary(const string& doctorName) const {
        vector<const DocPrimaryIndexEntry*> results;
        auto head_it = secondaryIndexHeads.find(doctorName);
        if (head_it == secondaryIndexHeads.end()) return results;

        int currentIdx = head_it->second;
        while (currentIdx != -1) {
            int primaryPos = secondaryIndex[currentIdx].primaryIndexPos;
            results.push_back(&primaryIndex[primaryPos]);
            currentIdx = secondaryIndex[currentIdx].next;
        }
        return results;
    }
};

#endif // INDEX_MANAGERS_H