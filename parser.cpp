#include <iostream>
#include <queue>
#include <sstream>

using namespace std;

class Parser {
private:
  string trim(string source) {
    string s(source);
    s.erase(0, s.find_first_not_of(" \n\r\t"));
    s.erase(s.find_last_not_of(" \n\r\t") + 1);
    return s;
  }

  string toLower(string source) {
    string s = source;
    for (char &c : s) {
      c = tolower(c);
    }
    return s;
  }

  void setQueue(string input) {
    queue<string> q;
    stringstream ss(input);
    while (ss >> input) {
      q.push(input);
    }
    this->stringQueue = q;
  }

  void setSelectFields() {
    while (!this->stringQueue.empty()) {
      string select = toLower(trim(this->stringQueue.front()));
      if (toLower(select) == "from")
        break;
      this->stringQueue.pop();
      if (select.back() == ',')
        select.pop_back();
      this->selectFields.push_back(select);
    }
  }

  void setWhereClause() {
    string whereClause = stringQueue.front();
    stringQueue.pop();
    if (!stringQueue.empty()) {
      throw invalid_argument("invalid statement after where clause");
    }
    // Remove semicolon if exists
    if (!whereClause.empty() && whereClause.back() == ';') {
      whereClause.pop_back();
    }

    // Find the '=' sign
    size_t equalPos = whereClause.find('=');
    if (equalPos == string::npos) {
      throw invalid_argument("invalid WHERE clause format");
    }

    // Extract column name and value
    this->columnName = trim(whereClause.substr(0, equalPos));
    this->columnValue = trim(whereClause.substr(equalPos + 1));

    // Remove quotes from value
    if (columnValue.length() >= 2 && columnValue.front() == '\'' &&
        columnValue.back() == '\'') {
      columnValue = columnValue.substr(1, columnValue.length() - 2);
    }
  }

public:
  vector<string> selectFields;
  string tableName;
  string columnName;
  string columnValue;
  queue<string> stringQueue;

  void parser(string input) {
    this->selectFields.clear();
    this->tableName = "";
    this->columnName = "";
    this->columnValue = "";
    this->stringQueue = queue<string>();
    setQueue(input);

    string firstStatement = toLower(trim(stringQueue.front()));
    stringQueue.pop();
    if (firstStatement != "select")
      throw invalid_argument("query must start with select statement");

    setSelectFields();

    if (toLower(trim(stringQueue.front())) != "from")
      throw invalid_argument("missing from statement");
    stringQueue.pop();

    if (toLower(trim(stringQueue.front())) == "where")
      throw invalid_argument("missing table name");

    this->tableName = trim(stringQueue.front());
    stringQueue.pop();
    if (stringQueue.empty()) {
      return;
    }
    if (toLower(trim(stringQueue.front())) != "where") {
      throw invalid_argument("invalid statement");
    }
    stringQueue.pop();
    setWhereClause();
  }
};

int main() {
  try {
    Parser parser;
    parser.parser("SELECT name, age FROM users WHERE id='123';");
    cout << "Table Name: " << parser.tableName << "\n";
    cout << "Select Fields: ";
    for (string field : parser.selectFields) {
      cout << field << " ";
    }
    cout << "where " << parser.columnName << " = " << parser.columnValue
         << "\n";
  } catch (const exception &e) {
    cerr << "Error: " << e.what() << "\n";
  }
  return 0;
}
