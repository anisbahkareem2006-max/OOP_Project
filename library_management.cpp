#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <ctime>
#include <memory>

using namespace std;

string currentDate() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    ostringstream oss;
    oss << (now->tm_year + 1900) << "-"
        << setw(2) << setfill('0') << (now->tm_mon + 1) << "-"
        << setw(2) << setfill('0') << now->tm_mday;
    return oss.str();
}

class LibraryException : public runtime_error {
public:
    explicit LibraryException(const string& msg) : runtime_error(msg) {}
};

class BookNotFoundException : public LibraryException {
public:
    explicit BookNotFoundException(const string& id)
        : LibraryException("Book not found: " + id) {}
};

class MemberNotFoundException : public LibraryException {
public:
    explicit MemberNotFoundException(const string& id)
        : LibraryException("Member not found: " + id) {}
};

class BookNotAvailableException : public LibraryException {
public:
    explicit BookNotAvailableException(const string& title)
        : LibraryException("Book not available for borrowing: " + title) {}
};

class LibraryItem {
protected:
    string itemID;
    string title;
    string author;
    int year;
    bool available;

public:
    LibraryItem(const string& id, const string& t, const string& a, int y)
        : itemID(id), title(t), author(a), year(y), available(true) {}

    virtual ~LibraryItem() = default;

    virtual string getType() const = 0;
    virtual void display() const = 0;
    virtual string serialize() const = 0;

    string getID()       const { return itemID; }
    string getTitle()    const { return title; }
    string getAuthor()   const { return author; }
    int    getYear()     const { return year; }
    bool   isAvailable() const { return available; }

    void setAvailable(bool status) { available = status; }

    bool operator<(const LibraryItem& other) const { return title < other.title; }
    bool operator==(const LibraryItem& other) const { return itemID == other.itemID; }

    friend ostream& operator<<(ostream& os, const LibraryItem& item) {
        os << "[" << item.getType() << "] " << item.title
           << " by " << item.author << " (" << item.year << ")"
           << " | " << (item.available ? "Available" : "Borrowed");
        return os;
    }
};

class Book : public LibraryItem {
private:
    string genre;
    int pages;

public:
    Book(const string& id, const string& t, const string& a,
         int y, const string& g, int p)
        : LibraryItem(id, t, a, y), genre(g), pages(p) {}

    string getType() const override { return "Book"; }
    string getGenre() const { return genre; }
    int    getPages() const { return pages; }

    void display() const override {
        cout << "  ID     : " << itemID  << "\n"
             << "  Title  : " << title   << "\n"
             << "  Author : " << author  << "\n"
             << "  Year   : " << year    << "\n"
             << "  Genre  : " << genre   << "\n"
             << "  Pages  : " << pages   << "\n"
             << "  Status : " << (available ? "Available" : "Borrowed") << "\n";
    }

    string serialize() const override {
        return "BOOK|" + itemID + "|" + title + "|" + author + "|" +
               to_string(year) + "|" + genre + "|" + to_string(pages) +
               "|" + (available ? "1" : "0");
    }

    static shared_ptr<Book> deserialize(const string& line) {
        istringstream ss(line);
        string token, id, t, a, g;
        int y, p;
        bool avail;
        getline(ss, token, '|');
        getline(ss, id,    '|');
        getline(ss, t,     '|');
        getline(ss, a,     '|');
        getline(ss, token, '|'); y = stoi(token);
        getline(ss, g,     '|');
        getline(ss, token, '|'); p = stoi(token);
        getline(ss, token, '|'); avail = (token == "1");
        auto book = make_shared<Book>(id, t, a, y, g, p);
        book->setAvailable(avail);
        return book;
    }
};

class Magazine : public LibraryItem {
private:
    int issueNumber;
    string publisher;

public:
    Magazine(const string& id, const string& t, const string& a,
             int y, int issue, const string& pub)
        : LibraryItem(id, t, a, y), issueNumber(issue), publisher(pub) {}

    string getType() const override { return "Magazine"; }

    void display() const override {
        cout << "  ID        : " << itemID      << "\n"
             << "  Title     : " << title       << "\n"
             << "  Editor    : " << author      << "\n"
             << "  Year      : " << year        << "\n"
             << "  Issue     : " << issueNumber << "\n"
             << "  Publisher : " << publisher   << "\n"
             << "  Status    : " << (available ? "Available" : "Borrowed") << "\n";
    }

    string serialize() const override {
        return "MAGAZINE|" + itemID + "|" + title + "|" + author + "|" +
               to_string(year) + "|" + to_string(issueNumber) + "|" + publisher +
               "|" + (available ? "1" : "0");
    }

    static shared_ptr<Magazine> deserialize(const string& line) {
        istringstream ss(line);
        string token, id, t, a, pub;
        int y, issue;
        bool avail;
        getline(ss, token, '|');
        getline(ss, id,    '|');
        getline(ss, t,     '|');
        getline(ss, a,     '|');
        getline(ss, token, '|'); y = stoi(token);
        getline(ss, token, '|'); issue = stoi(token);
        getline(ss, pub,   '|');
        getline(ss, token, '|'); avail = (token == "1");
        auto mag = make_shared<Magazine>(id, t, a, y, issue, pub);
        mag->setAvailable(avail);
        return mag;
    }
};

template<typename T>
class Repository {
private:
    map<string, T> store;

public:
    void add(const string& key, const T& val) { store[key] = val; }

    void remove(const string& key) {
        if (store.find(key) == store.end())
            throw LibraryException("Key not found: " + key);
        store.erase(key);
    }

    T get(const string& key) const {
        auto it = store.find(key);
        if (it == store.end())
            throw LibraryException("Key not found: " + key);
        return it->second;
    }

    bool exists(const string& key) const { return store.count(key) > 0; }
    const map<string, T>& getAll() const { return store; }
    int count() const { return (int)store.size(); }
};

struct BorrowRecord {
    string memberID;
    string itemID;
    string borrowDate;
    string returnDate;
    double fine;

    BorrowRecord(const string& mid, const string& iid, const string& bd)
        : memberID(mid), itemID(iid), borrowDate(bd), fine(0.0) {}

    string serialize() const {
        return memberID + "|" + itemID + "|" + borrowDate + "|" +
               (returnDate.empty() ? "NONE" : returnDate) + "|" + to_string(fine);
    }
};

class Member {
protected:
    string memberID;
    string name;
    string email;
    int    maxBorrow;
    vector<string> currentBorrows;

public:
    Member(const string& id, const string& n, const string& e, int maxB)
        : memberID(id), name(n), email(e), maxBorrow(maxB) {}

    virtual ~Member() = default;

    virtual string getRole() const = 0;
    virtual double fineRate() const = 0;

    string getID()          const { return memberID; }
    string getName()        const { return name; }
    string getEmail()       const { return email; }
    int    getMaxBorrow()   const { return maxBorrow; }
    int    getBorrowCount() const { return (int)currentBorrows.size(); }
    bool   canBorrow()      const { return (int)currentBorrows.size() < maxBorrow; }

    void addBorrow(const string& itemID) {
        if (!canBorrow())
            throw LibraryException(name + " has reached borrowing limit.");
        currentBorrows.push_back(itemID);
    }

    void removeBorrow(const string& itemID) {
        auto it = find(currentBorrows.begin(), currentBorrows.end(), itemID);
        if (it == currentBorrows.end())
            throw LibraryException("Borrow record not found for item: " + itemID);
        currentBorrows.erase(it);
    }

    const vector<string>& getBorrows() const { return currentBorrows; }

    virtual void display() const {
        cout << "  ID       : " << memberID              << "\n"
             << "  Name     : " << name                  << "\n"
             << "  Email    : " << email                 << "\n"
             << "  Role     : " << getRole()             << "\n"
             << "  Limit    : " << maxBorrow             << " items\n"
             << "  Borrowed : " << currentBorrows.size() << " items\n";
    }

    string serializeBase() const {
        string borrows;
        for (size_t i = 0; i < currentBorrows.size(); ++i)
            borrows += currentBorrows[i] + (i + 1 < currentBorrows.size() ? "," : "");
        return memberID + "|" + name + "|" + email + "|" +
               to_string(maxBorrow) + "|" + (borrows.empty() ? "NONE" : borrows);
    }

    bool operator==(const Member& other) const { return memberID == other.memberID; }

    friend ostream& operator<<(ostream& os, const Member& m) {
        os << m.getRole() << " | " << m.memberID << " | " << m.name;
        return os;
    }
};

class StudentMember : public Member {
private:
    string studentID;

public:
    StudentMember(const string& id, const string& n, const string& e, const string& sid)
        : Member(id, n, e, 3), studentID(sid) {}

    string getRole()  const override { return "Student"; }
    double fineRate() const override { return 5.0; }

    void display() const override {
        Member::display();
        cout << "  StudentID: " << studentID << "\n"
             << "  Fine Rate: Rs. " << fineRate() << "/day\n";
    }

    string serialize() const { return "STUDENT|" + serializeBase() + "|" + studentID; }
};

class FacultyMember : public Member {
private:
    string department;

public:
    FacultyMember(const string& id, const string& n, const string& e, const string& dept)
        : Member(id, n, e, 7), department(dept) {}

    string getRole()  const override { return "Faculty"; }
    double fineRate() const override { return 2.0; }

    void display() const override {
        Member::display();
        cout << "  Dept     : " << department << "\n"
             << "  Fine Rate: Rs. " << fineRate() << "/day\n";
    }

    string serialize() const { return "FACULTY|" + serializeBase() + "|" + department; }
};

class Library {
private:
    string name;
    Repository<shared_ptr<LibraryItem>> items;
    Repository<shared_ptr<Member>>      members;
    vector<BorrowRecord>                borrowHistory;

    shared_ptr<LibraryItem> findItem(const string& id) const {
        if (!items.exists(id)) throw BookNotFoundException(id);
        return items.get(id);
    }

    shared_ptr<Member> findMember(const string& id) const {
        if (!members.exists(id)) throw MemberNotFoundException(id);
        return members.get(id);
    }

    int daysBetween(const string& from, const string& to) const {
        auto parse = [](const string& s) {
            return stoi(s.substr(0,4)) * 365 + stoi(s.substr(5,2)) * 30 + stoi(s.substr(8,2));
        };
        return max(0, parse(to) - parse(from));
    }

public:
    explicit Library(const string& n) : name(n) {}

    void addItem(shared_ptr<LibraryItem> item) {
        if (items.exists(item->getID()))
            throw LibraryException("Item ID already exists: " + item->getID());
        items.add(item->getID(), item);
        cout << "  [+] Added: " << *item << "\n";
    }

    void removeItem(const string& id) {
        auto item = findItem(id);
        if (!item->isAvailable())
            throw LibraryException("Cannot remove a borrowed item.");
        items.remove(id);
        cout << "  [-] Removed item: " << id << "\n";
    }

    void registerMember(shared_ptr<Member> m) {
        if (members.exists(m->getID()))
            throw LibraryException("Member ID already exists: " + m->getID());
        members.add(m->getID(), m);
        cout << "  [+] Registered: " << *m << "\n";
    }

    void borrowItem(const string& memberID, const string& itemID) {
        auto member = findMember(memberID);
        auto item   = findItem(itemID);

        if (!item->isAvailable())
            throw BookNotAvailableException(item->getTitle());
        if (!member->canBorrow())
            throw LibraryException(member->getName() + " has reached borrow limit.");

        item->setAvailable(false);
        member->addBorrow(itemID);
        borrowHistory.emplace_back(memberID, itemID, currentDate());

        cout << "  [✓] \"" << item->getTitle() << "\" borrowed by "
             << member->getName() << " on " << currentDate() << "\n";
    }

    void returnItem(const string& memberID, const string& itemID,
                    const string& returnDate = "") {
        auto member  = findMember(memberID);
        auto item    = findItem(itemID);
        string rDate = returnDate.empty() ? currentDate() : returnDate;

        BorrowRecord* rec = nullptr;
        for (auto& r : borrowHistory)
            if (r.memberID == memberID && r.itemID == itemID && r.returnDate.empty()) {
                rec = &r; break;
            }

        if (!rec) throw LibraryException("No active borrow record found.");

        rec->returnDate = rDate;
        int days        = daysBetween(rec->borrowDate, rDate);
        int allowedDays = (member->getRole() == "Student") ? 14 : 30;
        int lateDays    = max(0, days - allowedDays);
        rec->fine       = lateDays * member->fineRate();

        item->setAvailable(true);
        member->removeBorrow(itemID);

        cout << "  [✓] \"" << item->getTitle() << "\" returned by "
             << member->getName() << "\n"
             << "      Days held : " << days << " | Late days: " << lateDays
             << " | Fine: Rs. " << fixed << setprecision(2) << rec->fine << "\n";
    }

    void searchByTitle(const string& keyword) const {
        cout << "\n  Search results for \"" << keyword << "\":\n";
        bool found = false;
        for (auto it = items.getAll().begin(); it != items.getAll().end(); ++it) {
            string t = it->second->getTitle(), kw = keyword;
            transform(t.begin(),  t.end(),  t.begin(),  ::tolower);
            transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
            if (t.find(kw) != string::npos) { cout << "  " << *(it->second) << "\n"; found = true; }
        }
        if (!found) cout << "  No items found.\n";
    }

    void searchByAuthor(const string& author) const {
        cout << "\n  Books by \"" << author << "\":\n";
        bool found = false;
        for (auto it = items.getAll().begin(); it != items.getAll().end(); ++it) {
            string a = it->second->getAuthor(), kw = author;
            transform(a.begin(),  a.end(),  a.begin(),  ::tolower);
            transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
            if (a.find(kw) != string::npos) { cout << "  " << *(it->second) << "\n"; found = true; }
        }
        if (!found) cout << "  No items found.\n";
    }

    void displayAllItems() const {
        cout << "\n  === All Library Items (" << items.count() << ") ===\n";
        for (auto it = items.getAll().begin(); it != items.getAll().end(); ++it)
            cout << "  " << *(it->second) << "\n";
    }

    void displayAllMembers() const {
        cout << "\n  === All Members (" << members.count() << ") ===\n";
        for (auto it = members.getAll().begin(); it != members.getAll().end(); ++it)
            cout << "  " << *(it->second) << "\n";
    }

    void displayMemberDetail(const string& id) const {
        auto m = findMember(id);
        cout << "\n  -- Member Details --\n";
        m->display();
        if (!m->getBorrows().empty()) {
            cout << "  Currently borrowing:\n";
            for (auto& bid : m->getBorrows())
                if (items.exists(bid))
                    cout << "    - " << items.get(bid)->getTitle() << "\n";
        }
    }

    void displayBorrowHistory() const {
        cout << "\n  === Borrow History ===\n"
             << left << setw(10) << "Member" << setw(10) << "Item"
             << setw(14) << "Borrowed" << setw(14) << "Returned" << "Fine\n"
             << string(58, '-') << "\n";
        for (auto& r : borrowHistory)
            cout << left << setw(10) << r.memberID << setw(10) << r.itemID
                 << setw(14) << r.borrowDate
                 << setw(14) << (r.returnDate.empty() ? "Active" : r.returnDate)
                 << "Rs. " << fixed << setprecision(2) << r.fine << "\n";
    }

    void displayStats() const {
        int total = items.count(), available = 0;
        for (auto it = items.getAll().begin(); it != items.getAll().end(); ++it)
            if (it->second->isAvailable()) available++;
        double totalFines = 0;
        for (auto& r : borrowHistory) totalFines += r.fine;

        cout << "\n  === Library Statistics ===\n"
             << "  Name         : " << name                << "\n"
             << "  Total Items  : " << total               << "\n"
             << "  Available    : " << available           << "\n"
             << "  Borrowed     : " << (total - available) << "\n"
             << "  Members      : " << members.count()     << "\n"
             << "  Transactions : " << borrowHistory.size()<< "\n"
             << "  Total Fines  : Rs. " << fixed << setprecision(2) << totalFines << "\n";
    }

    void saveToFile(const string& filename) const {
        ofstream f(filename);
        if (!f) throw LibraryException("Cannot open file: " + filename);
        for (auto it = items.getAll().begin(); it != items.getAll().end(); ++it)
            f << it->second->serialize() << "\n";
        for (auto it = members.getAll().begin(); it != members.getAll().end(); ++it) {
            if (auto sm = dynamic_pointer_cast<StudentMember>(it->second))
                f << sm->serialize() << "\n";
            else if (auto fm = dynamic_pointer_cast<FacultyMember>(it->second))
                f << fm->serialize() << "\n";
        }
        for (auto& r : borrowHistory)
            f << "RECORD|" << r.serialize() << "\n";
        cout << "  [✓] Data saved to " << filename << "\n";
    }

    void loadFromFile(const string& filename) {
        ifstream f(filename);
        if (!f) { cout << "  [!] No save file found. Starting fresh.\n"; return; }
        string line;
        while (getline(f, line)) {
            if (line.empty()) continue;
            string type = line.substr(0, line.find('|'));
            try {
                if (type == "BOOK") {
                    items.add(line.substr(5, line.find('|', 5) - 5), Book::deserialize(line));
                } else if (type == "MAGAZINE") {
                    auto mag = Magazine::deserialize(line);
                    items.add(mag->getID(), mag);
                } else if (type == "STUDENT") {
                    istringstream ss(line.substr(7));
                    string id, n, e, borrows, sid, tok;
                    getline(ss, id, '|'); getline(ss, n, '|'); getline(ss, e, '|');
                    getline(ss, tok, '|'); getline(ss, borrows, '|'); getline(ss, sid);
                    auto sm = make_shared<StudentMember>(id, n, e, sid);
                    if (borrows != "NONE") {
                        istringstream bs(borrows); string bid;
                        while (getline(bs, bid, ',')) sm->addBorrow(bid);
                    }
                    members.add(id, sm);
                } else if (type == "FACULTY") {
                    istringstream ss(line.substr(7));
                    string id, n, e, borrows, dept, tok;
                    getline(ss, id, '|'); getline(ss, n, '|'); getline(ss, e, '|');
                    getline(ss, tok, '|'); getline(ss, borrows, '|'); getline(ss, dept);
                    auto fm = make_shared<FacultyMember>(id, n, e, dept);
                    if (borrows != "NONE") {
                        istringstream bs(borrows); string bid;
                        while (getline(bs, bid, ',')) fm->addBorrow(bid);
                    }
                    members.add(id, fm);
                } else if (type == "RECORD") {
                    istringstream ss(line.substr(7));
                    string mid, iid, bd, rd, fine;
                    getline(ss, mid, '|'); getline(ss, iid, '|');
                    getline(ss, bd,  '|'); getline(ss, rd,  '|'); getline(ss, fine);
                    BorrowRecord rec(mid, iid, bd);
                    rec.returnDate = (rd == "NONE") ? "" : rd;
                    rec.fine = stod(fine);
                    borrowHistory.push_back(rec);
                }
            } catch (...) {}
        }
        cout << "  [✓] Data loaded from " << filename << "\n";
    }
};

void printHeader(const string& title) {
    cout << "\n" << string(50, '=') << "\n  " << title << "\n" << string(50, '=') << "\n";
}

int main() {
    Library lib("FAST-NUCES Library");
    const string SAVE_FILE = "library_data.txt";

    printHeader("Loading Data");
    lib.loadFromFile(SAVE_FILE);

    printHeader("Adding Library Items");
    try {
        lib.addItem(make_shared<Book>("B001", "Clean Code", "Robert C. Martin", 2008, "Programming", 431));
        lib.addItem(make_shared<Book>("B002", "The Pragmatic Programmer", "Andrew Hunt", 1999, "Programming", 352));
        lib.addItem(make_shared<Book>("B003", "Design Patterns", "Gang of Four", 1994, "Software Engineering", 395));
        lib.addItem(make_shared<Book>("B004", "Introduction to Algorithms", "Cormen et al.", 2009, "Computer Science", 1312));
        lib.addItem(make_shared<Magazine>("M001", "IEEE Spectrum", "IEEE Editorial", 2024, 62, "IEEE"));
        lib.addItem(make_shared<Magazine>("M002", "ACM Communications", "ACM Board", 2024, 47, "ACM"));
    } catch (LibraryException& e) {
        cout << "  [!] " << e.what() << "\n";
    }

    printHeader("Registering Members");
    try {
        lib.registerMember(make_shared<StudentMember>("S001", "Adeel Khan", "adeel@student.edu", "FA21-BSE-001"));
        lib.registerMember(make_shared<StudentMember>("S002", "Sara Ahmed", "sara@student.edu", "FA21-BSE-042"));
        lib.registerMember(make_shared<FacultyMember>("F001", "Dr. Tariq Mahmood", "tariq@fast.edu", "CS Department"));
        lib.registerMember(make_shared<FacultyMember>("F002", "Dr. Ayesha Bano", "ayesha@fast.edu", "SE Department"));
    } catch (LibraryException& e) {
        cout << "  [!] " << e.what() << "\n";
    }

    lib.displayAllItems();
    lib.displayAllMembers();

    printHeader("Borrowing Items");
    try {
        lib.borrowItem("S001", "B001");
        lib.borrowItem("S001", "B002");
        lib.borrowItem("F001", "B003");
        lib.borrowItem("F001", "M001");
        lib.borrowItem("S002", "B004");
        lib.borrowItem("S002", "B001");
    } catch (LibraryException& e) {
        cout << "  [!] Exception caught: " << e.what() << "\n";
    }

    printHeader("Searching");
    lib.searchByTitle("code");
    lib.searchByAuthor("cormen");

    printHeader("Returning Items");
    try {
        lib.returnItem("S001", "B001", "2026-07-15");
        lib.returnItem("F001", "B003", "2026-07-10");
    } catch (LibraryException& e) {
        cout << "  [!] " << e.what() << "\n";
    }

    printHeader("Member Detail");
    lib.displayMemberDetail("S001");
    lib.displayMemberDetail("F001");

    lib.displayBorrowHistory();
    lib.displayStats();

    printHeader("Item Detail");
    lib.searchByTitle("Design Patterns");

    printHeader("Remove Item");
    try {
        lib.removeItem("M002");
        lib.removeItem("B002");
    } catch (LibraryException& e) {
        cout << "  [!] " << e.what() << "\n";
    }

    printHeader("Saving Data");
    try {
        lib.saveToFile(SAVE_FILE);
    } catch (LibraryException& e) {
        cout << "  [!] " << e.what() << "\n";
    }

    cout << "\n" << string(50, '=') << "\n  Program ended successfully.\n" << string(50, '=') << "\n";
    return 0;
}
