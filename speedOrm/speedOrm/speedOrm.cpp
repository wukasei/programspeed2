#include <iostream>
#include "orm.h"


using namespace std;

using namespace odb::core;

int main() {

    try {
        auto db = connectDB();
        int choice;

        do {
            cout << "\n--- MENU ---\n";
            cout << "1. Average SELECT all models\n";
            cout << "2. Average insert Order model\n";
            cout << "3. Average update Order model\n";
            cout << "4. Average delete Order model\n";
            cout << "0. Exit\n> ";
            cin >> choice;

            switch (choice) {
            case 1: {
                int limit, repeats;
                cout << "Rows limit per table: "; cin >> limit;
                cout << "Repeats: "; cin >> repeats;
                averageSelectAllModels(db, limit, repeats);
                break;
            }
            case 2: {
                int repeats;
                cout << "Repeats: "; cin >> repeats;
                benchmarkInsertOrders(db, repeats);
                break;
            }
            case 3: {
                int repeats;
                cout << "Repeats: "; cin >> repeats;
                benchmarkUpdateOrders(db, repeats);
                break;
            }
            case 4: {
                int repeats;
                cout << "Repeats: "; cin >> repeats;
                benchmarkDeleteOrders(db, repeats);
                break;
            }
            case 0:
                cout << "Exiting...\n";
                break;
            default:
                cout << "Invalid choice!\n";
                break;
            }
        } while (choice != 0);
    }
    catch (const odb::exception& e) {

        cerr << "ODB error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
