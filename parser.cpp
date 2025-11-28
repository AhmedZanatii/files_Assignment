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
      if (select == "from")
        break;
      this->stringQueue.pop();
      if (select.back() == ',')
        select.pop_back();
      this->selectFields.push_back(select);
    }
  }

  void setWhereClause() {
    // Concatenate all remaining tokens into whereClause
    string whereClause = "";
    while (!stringQueue.empty()) {
      if (!whereClause.empty()) {
        whereClause += " ";
      }
      whereClause += stringQueue.front();
      stringQueue.pop();
    }
    whereClause = trim(whereClause);

    // Remove semicolon if exists
    if (!whereClause.empty() && whereClause.back() == ';') {
      whereClause.pop_back();
    }

    // Find the '=' sign
    size_t equalPos = whereClause.find('=');
    if (equalPos == string::npos) {
      throw invalid_argument("invalid WHERE clause format");
    }

    // Extract column name (lowercase)
    this->searchColumnName = toLower(trim(whereClause.substr(0, equalPos)));
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
  string searchColumnName;
  string columnValue;
  queue<string> stringQueue;

  void parse(string input) {
    this->selectFields.clear();
    this->tableName = "";
    this->searchColumnName = "";
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
