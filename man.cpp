#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <ctime>
#include <unordered_map>
#include <vector>

using namespace std;

// Represents the lifecycle of a support ticket.
// A ticket normally starts as OPEN, moves to IN_PROGRESS, and can finally become RESOLVED.
enum class Status {
    OPEN,
    IN_PROGRESS,
    RESOLVED
};

string statusToString(Status status) {
    switch (status) {
        case Status::OPEN: return "Open";
        case Status::IN_PROGRESS: return "In Progress";
        case Status::RESOLVED: return "Resolved";
    }
    return "Unknown";
}

// Stores all information related to one customer support ticket.
// Keeping these fields together makes a ticket easy to create, search, update, and display.
struct Ticket {
    int id;
    string customer;
    string issue;
    int priority; // 1 = High, 2 = Medium, 3 = Low
    Status status;
    time_t createdAt; // Stores the date/time when the ticket was created.

    Ticket(int id, string customer, string issue, int priority)
        : id(id), customer(std::move(customer)), issue(std::move(issue)),
          priority(priority), status(Status::OPEN), createdAt(time(nullptr)) {}
};

// Defines the rule used by priority_queue to decide which ticket should be processed first.
// Lower priority number means higher urgency: 1 (High) -> 2 (Medium) -> 3 (Low).
// If two tickets have the same priority, the smaller ID is processed first.
struct TicketCompare {
    bool operator()(const Ticket* a, const Ticket* b) const {
        if (a->priority != b->priority)
            return a->priority > b->priority; // High priority first
        return a->id > b->id;                 // Older/smaller ID first
    }
};

// Handles the main business logic of the ticket system.
// The unordered_map gives fast lookup of a ticket by its unique ID.
class TicketManager {
private:
    unordered_map<int, Ticket> tickets;
    int nextId = 1001;

    string priorityToString(int priority) const {
        if (priority == 1) return "High";
        if (priority == 2) return "Medium";
        return "Low";
    }

    // Converts the stored creation timestamp into a readable date/time string.
    string formatTimestamp(time_t timestamp) const {
        tm* localTime = localtime(&timestamp);
        if (localTime == nullptr) return "Unknown";

        char buffer[80];
        strftime(buffer, sizeof(buffer), "%d-%b-%Y %I:%M %p", localTime);
        return string(buffer);
    }

public:
    // Creates a new ticket, assigns the next available ID, and stores it in the map.
    int createTicket(const string& customer, const string& issue, int priority) {
        int id = nextId++;
        tickets.emplace(id, Ticket(id, customer, issue, priority));
        return id;
    }

    // Returns a pointer to the requested ticket, or nullptr when the ID does not exist.
    // A pointer is used so the caller can work directly with the stored ticket object.
    Ticket* findTicket(int id) {
        auto it = tickets.find(id);
        if (it == tickets.end()) return nullptr;
        return &it->second;
    }

    void displayTicket(const Ticket& ticket) const {
        cout << "\n----------------------------------------\n";
        cout << "Ticket ID : " << ticket.id << '\n';
        cout << "Customer  : " << ticket.customer << '\n';
        cout << "Issue     : " << ticket.issue << '\n';
        cout << "Priority  : " << priorityToString(ticket.priority) << '\n';
        cout << "Status    : " << statusToString(ticket.status) << '\n';
        cout << "Created At: " << formatTimestamp(ticket.createdAt) << '\n';
        cout << "----------------------------------------\n";
    }

    // Displays all tickets sorted by priority and then by ticket ID.
    // The map itself is not sorted, so a temporary vector is created and sorted for display.
    void listTickets() const {
        if (tickets.empty()) {
            cout << "\nNo tickets available.\n";
            return;
        }

        vector<const Ticket*> sorted;
        for (const auto& [id, ticket] : tickets)
            sorted.push_back(&ticket);

        sort(sorted.begin(), sorted.end(),
             [](const Ticket* a, const Ticket* b) {
                 if (a->priority != b->priority)
                     return a->priority < b->priority;
                 return a->id < b->id;
             });

        cout << "\nAll Tickets\n";
        cout << left << setw(8) << "ID"
             << setw(20) << "Customer"
             << setw(12) << "Priority"
             << setw(15) << "Status"
             << "Issue\n";
        cout << string(75, '-') << '\n';

        for (const Ticket* ticket : sorted) {
            cout << left << setw(8) << ticket->id
                 << setw(20) << ticket->customer
                 << setw(12) << priorityToString(ticket->priority)
                 << setw(15) << statusToString(ticket->status)
                 << ticket->issue << '\n';
        }
    }

    void searchTicket(int id) const {
        auto it = tickets.find(id);
        if (it == tickets.end()) {
            cout << "\nTicket not found.\n";
            return;
        }
        displayTicket(it->second);
    }

    void updateStatus(int id, Status newStatus) {
        auto it = tickets.find(id);
        if (it == tickets.end()) {
            cout << "\nTicket not found.\n";
            return;
        }

        it->second.status = newStatus;
        cout << "\nTicket status updated successfully.\n";
    }

    // Finds the most important unresolved ticket and marks it as IN_PROGRESS
    // priority_queue is rebuilt from the current tickets so the latest statuses are considered.
    void processNextTicket() {
        priority_queue<Ticket*, vector<Ticket*>, TicketCompare> pq;

        for (auto& [id, ticket] : tickets) {
            if (ticket.status != Status::RESOLVED)
                pq.push(&ticket);
        }

        if (pq.empty()) {
            cout << "\nNo pending tickets.\n";
            return;
        }

        Ticket* next = pq.top();
        cout << "\nNext ticket to process:";
        displayTicket(*next);

        next->status = Status::IN_PROGRESS;
        cout << "Status changed to In Progress.\n";
    }

    // Searches all tickets whose customer name matches the entered name.
    // The comparison is case-insensitive so "aarav" and "Aarav" both work.
    void searchByCustomer(const string& customerName) const {
        bool found = false;

        for (const auto& [id, ticket] : tickets) {
            string storedName = ticket.customer;
            string searchName = customerName;

            transform(storedName.begin(), storedName.end(), storedName.begin(),
                      [](unsigned char c) { return static_cast<char>(tolower(c)); });

            transform(searchName.begin(), searchName.end(), searchName.begin(),
                      [](unsigned char c) { return static_cast<char>(tolower(c)); });

            if (storedName == searchName) {
                displayTicket(ticket);
                found = true;
            }
        }
        //other condition
        if (!found)
            cout << "\nNo tickets found for customer: " << customerName << '\n';
    }

    // Deletes a ticket permanently from the system using its ID.
    void deleteTicket(int id) {
        auto it = tickets.find(id);

        if (it == tickets.end()) {
            cout << "\nTicket not found.\n";
            return;
        }

        tickets.erase(it);
        cout << "\nTicket deleted successfully.\n";
    }

    // Marks a ticket as resolved instead of deleting it.
    // This is useful when we want to keep the ticket history.
    void closeTicket(int id) {
        auto it = tickets.find(id);

        if (it == tickets.end()) {
            cout << "\nTicket not found.\n";
            return;
        }

        it->second.status = Status::RESOLVED;
        cout << "\nTicket closed successfully.\n";
    }

    // Counts tickets in each status to provide a quick overview of system workload.
    void showStatistics() const {
        int open = 0, inProgress = 0, resolved = 0;

        for (const auto& [id, ticket] : tickets) {
            if (ticket.status == Status::OPEN) open++;
            else if (ticket.status == Status::IN_PROGRESS) inProgress++;
            else resolved++;
        }

        cout << "\nTicket Statistics\n";
        cout << "Open        : " << open << '\n';
        cout << "In Progress : " << inProgress << '\n';
        cout << "Resolved    : " << resolved << '\n';
        cout << "Total       : " << tickets.size() << '\n';
    }
};

// Reads an integer and keeps asking until the value falls inside the allowed range.
// It also clears invalid input from cin so the next read can work normally.
int readInt(const string& prompt, int minValue, int maxValue) {
    int value;
    // Keep showing the menu until the user chooses the Exit option.
    while (true) {
        cout << prompt;
        if (cin >> value && value >= minValue && value <= maxValue) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }

        cout << "Invalid input. Please try again.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int main() {
    TicketManager manager;

    // Add a few sample tickets so the system has data available as soon as it starts.
    // This makes it easier to demonstrate the features without creating tickets manually.
    manager.createTicket("Aarav", "Unable to login", 1);
    manager.createTicket("Riya", "Payment failed", 2);
    manager.createTicket("Kabir", "Update profile details", 3);

    while (true) {
        cout << "\n========== CUSTOMER SUPPORT TICKET SYSTEM ==========\n";
        cout << "1. Create Ticket\n";
        cout << "2. View All Tickets\n";
        cout << "3. Search Ticket by ID\n";
        cout << "4. Search Tickets by Customer Name\n";
        cout << "5. Update Ticket Status\n";
        cout << "6. Process Next Priority Ticket\n";
        cout << "7. Close Ticket\n";
        cout << "8. Delete Ticket\n";
        cout << "9. Show Statistics\n";
        cout << "10. Exit\n";

        // readInt validates the menu choice and prevents invalid input from breaking the program.
        int choice = readInt("Enter choice: ", 1, 10);

        // Option 1: collect ticket details from the user and store the new ticket.
        if (choice == 1) {
            string customer, issue;
            cout << "Customer name: ";
            getline(cin, customer);

            cout << "Issue: ";
            getline(cin, issue);

            int priority = readInt("Priority (1=High, 2=Medium, 3=Low): ", 1, 3);

            int id = manager.createTicket(customer, issue, priority);
            cout << "Ticket created successfully. Ticket ID: " << id << '\n';

        // Option 2: display every ticket in priority order.
        } else if (choice == 2) {
            manager.listTickets();

        // Option 3: find and display one ticket using its unique ID.
        } else if (choice == 3) {
            int id = readInt("Enter ticket ID: ", 1000, 999999);
            manager.searchTicket(id);

        // Option 4: change the current status of an existing ticket.
        } else if (choice == 4) {
            int id = readInt("Enter ticket ID: ", 1000, 999999);
            int status = readInt("Status (1=Open, 2=In Progress, 3=Resolved): ", 1, 3);

            Status newStatus =
                status == 1 ? Status::OPEN :
                status == 2 ? Status::IN_PROGRESS :
                              Status::RESOLVED;

            manager.updateStatus(id, newStatus);

        // Option 5: select the highest-priority unresolved ticket for processing.
        } else if (choice == 5) {
            manager.processNextTicket();

        // Option 6: count tickets by status and show a quick summary.
        } else if (choice == 6) {
            manager.showStatistics();

        // Any remaining choice is 7, so end the program.
        } else {
            cout << "\nGood luck with your Salesforce interview!\n";
            break;
        }
    }

    return 0;
}
