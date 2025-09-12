#include "../inc/Serde.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace utils;
using namespace utils::serde;

// Example user-defined struct
struct Person {
    int mAge;
    float mHeight;
    std::string mName;
    std::vector<int> mFavoriteNumbers;

    Person() = default;
    Person(int age, float height, const std::string& name, const std::vector<int>& numbers)
        : mAge(age), mHeight(height), mName(name), mFavoriteNumbers(numbers) {}

    // Required serialize method for user-defined structs
    void serialize(std::vector<uint8_t>& buffer) const {
write(buffer, mAge);
write(buffer, mHeight);
write(buffer, mName);
write(buffer, mFavoriteNumbers);
    }

    // Required deserialize method for user-defined structs
    void deserialize(const std::vector<uint8_t>& buffer, std::size_t& offset) {
read(buffer, offset, mAge);
read(buffer, offset, mHeight);
read(buffer, offset, mName);
read(buffer, offset, mFavoriteNumbers);
    }

    bool operator==(const Person& other) const {
        return mAge == other.mAge &&
               mHeight == other.mHeight &&
               mName == other.mName &&
               mFavoriteNumbers == other.mFavoriteNumbers;
    }
};

// Example nested struct
struct Team {
    std::string mTeamName;
    std::vector<Person> mMembers;
    int mFoundedYear;

    Team() = default;
    Team(const std::string& name, const std::vector<Person>& members, int year)
        : mTeamName(name), mMembers(members), mFoundedYear(year) {}

    void serialize(std::vector<uint8_t>& buffer) const {
write(buffer, mTeamName);
write(buffer, mMembers);
write(buffer, mFoundedYear);
    }

    void deserialize(const std::vector<uint8_t>& buffer, std::size_t& offset) {
read(buffer, offset, mTeamName);
read(buffer, offset, mMembers);
read(buffer, offset, mFoundedYear);
    }

    bool operator==(const Team& other) const {
        return mTeamName == other.mTeamName &&
               mMembers == other.mMembers &&
               mFoundedYear == other.mFoundedYear;
    }
};

// Example using declarations and typedef
using PersonId = uint32_t;
using PersonName = std::string;
using ScoreType = float;

typedef std::vector<int> IntList;
typedef std::vector<Person> PersonList;
typedef std::vector<Team> TeamList;

// Example struct using typedef types
struct Employee {
    PersonId mId;
    PersonName mName;
    ScoreType mPerformanceScore;
    IntList mProjectIds;

    Employee() = default;
    Employee(PersonId id, const PersonName& name, ScoreType score, const IntList& projects)
        : mId(id), mName(name), mPerformanceScore(score), mProjectIds(projects) {}

    void serialize(std::vector<uint8_t>& buffer) const {
write(buffer, mId);
write(buffer, mName);
write(buffer, mPerformanceScore);
write(buffer, mProjectIds);
    }

    void deserialize(const std::vector<uint8_t>& buffer, std::size_t& offset) {
read(buffer, offset, mId);
read(buffer, offset, mName);
read(buffer, offset, mPerformanceScore);
read(buffer, offset, mProjectIds);
    }

    bool operator==(const Employee& other) const {
        return mId == other.mId &&
               mName == other.mName &&
               mPerformanceScore == other.mPerformanceScore &&
               mProjectIds == other.mProjectIds;
    }
};

// Complex struct with vectors of custom types
struct Department {
    std::string mDepartmentName;
    PersonList mPeople;          // typedef vector<Person>
    TeamList mTeams;             // typedef vector<Team>
    std::vector<Employee> mEmployees;  // regular vector<custom_type>
    IntList mBudgetAllocations;  // typedef vector<int>

    Department() = default;
    Department(const std::string& name, const PersonList& people, const TeamList& teams,
               const std::vector<Employee>& employees, const IntList& budget)
        : mDepartmentName(name), mPeople(people), mTeams(teams), mEmployees(employees), mBudgetAllocations(budget) {}

    void serialize(std::vector<uint8_t>& buffer) const {
write(buffer, mDepartmentName);
write(buffer, mPeople);
write(buffer, mTeams);
write(buffer, mEmployees);
write(buffer, mBudgetAllocations);
    }

    void deserialize(const std::vector<uint8_t>& buffer, std::size_t& offset) {
read(buffer, offset, mDepartmentName);
read(buffer, offset, mPeople);
read(buffer, offset, mTeams);
read(buffer, offset, mEmployees);
read(buffer, offset, mBudgetAllocations);
    }

    bool operator==(const Department& other) const {
        return mDepartmentName == other.mDepartmentName &&
               mPeople == other.mPeople &&
               mTeams == other.mTeams &&
               mEmployees == other.mEmployees &&
               mBudgetAllocations == other.mBudgetAllocations;
    }
};

void testPodTypes() {
    std::cout << "Testing POD types...\n";

    // Test basic POD types
    int intVal = 42;
    float floatVal = 3.14f;
    double doubleVal = 2.71828;
    char charVal = 'X';

    // Test serialization to buffer
    auto intBuffer = serialize(intVal);
    auto floatBuffer = serialize(floatVal);
    auto doubleBuffer = serialize(doubleVal);
    auto charBuffer = serialize(charVal);

    // Test deserialization from buffer
    auto intResult = deserialize<int>(intBuffer);
    auto floatResult = deserialize<float>(floatBuffer);
    auto doubleResult = deserialize<double>(doubleBuffer);
    auto charResult = deserialize<char>(charBuffer);

    assert(intVal == intResult);
    assert(floatVal == floatResult);
    assert(doubleVal == doubleResult);
    assert(charVal == charResult);

    // Test file I/O
saveToFile("test_int.bin", intVal);
saveToFile("test_float.bin", floatVal);

    auto loadedInt = loadFromFile<int>("test_int.bin");
    auto loadedFloat = loadFromFile<float>("test_float.bin");

    assert(intVal == loadedInt);
    assert(floatVal == loadedFloat);

    std::cout << "POD types test passed!\n";
}

void testStrings() {
    std::cout << "Testing strings...\n";

    std::string testStr = "Hello, Serde World! 🌍";
    std::string emptyStr = "";

    // Test serialization
    auto strBuffer = serialize(testStr);
    auto emptyBuffer = serialize(emptyStr);

    // Test deserialization
    auto strResult = deserialize<std::string>(strBuffer);
    auto emptyResult = deserialize<std::string>(emptyBuffer);

    assert(testStr == strResult);
    assert(emptyStr == emptyResult);

    // Test file I/O
saveToFile("test_string.bin", testStr);
    auto loadedStr = loadFromFile<std::string>("test_string.bin");
    assert(testStr == loadedStr);

    std::cout << "Strings test passed!\n";
}

void testVectors() {
    std::cout << "Testing vectors...\n";

    // Test vector of POD types
    std::vector<int> intVec = {1, 2, 3, 4, 5};
    std::vector<float> floatVec = {1.1f, 2.2f, 3.3f};
    std::vector<std::string> stringVec = {"hello", "world", "serde"};

    // Test serialization
    auto intVecBuffer = serialize(intVec);
    auto floatVecBuffer = serialize(floatVec);
    auto stringVecBuffer = serialize(stringVec);

    // Test deserialization
    auto intVecResult = deserialize<std::vector<int>>(intVecBuffer);
    auto floatVecResult = deserialize<std::vector<float>>(floatVecBuffer);
    auto stringVecResult = deserialize<std::vector<std::string>>(stringVecBuffer);

    assert(intVec == intVecResult);
    assert(floatVec == floatVecResult);
    assert(stringVec == stringVecResult);

    // Test file I/O
saveToFile("test_int_vector.bin", intVec);
saveToFile("test_string_vector.bin", stringVec);

    auto loadedIntVec = loadFromFile<std::vector<int>>("test_int_vector.bin");
    auto loadedStringVec = loadFromFile<std::vector<std::string>>("test_string_vector.bin");

    assert(intVec == loadedIntVec);
    assert(stringVec == loadedStringVec);

    std::cout << "Vectors test passed!\n";
}

void testUserDefinedStructs() {
    std::cout << "Testing user-defined structs...\n";

    // Create test data
    Person person1(25, 175.5f, "Alice", {1, 3, 7, 11});
    Person person2(30, 180.0f, "Bob", {2, 4, 8});

    // Test single struct
    auto personBuffer = serialize(person1);
    auto personResult = deserialize<Person>(personBuffer);
    assert(person1 == personResult);

    // Test vector of structs
    std::vector<Person> people = {person1, person2};
    auto peopleBuffer = serialize(people);
    auto peopleResult = deserialize<std::vector<Person>>(peopleBuffer);
    assert(people == peopleResult);

    // Test file I/O
saveToFile("test_person.bin", person1);
saveToFile("test_people.bin", people);

    auto loadedPerson = loadFromFile<Person>("test_person.bin");
    auto loadedPeople = loadFromFile<std::vector<Person>>("test_people.bin");

    assert(person1 == loadedPerson);
    assert(people == loadedPeople);

    std::cout << "User-defined structs test passed!\n";
}

void testNestedStructs() {
    std::cout << "Testing nested structs...\n";

    // Create test data
    Person alice(25, 175.5f, "Alice", {1, 3, 7});
    Person bob(30, 180.0f, "Bob", {2, 4, 8});
    Team team("Engineering", {alice, bob}, 2020);

    // Test serialization/deserialization
    auto teamBuffer = serialize(team);
    auto teamResult = deserialize<Team>(teamBuffer);
    assert(team == teamResult);

    // Test file I/O
saveToFile("test_team.bin", team);
    auto loadedTeam = loadFromFile<Team>("test_team.bin");
    assert(team == loadedTeam);

    // Test vector of teams
    Team team2("Marketing", {alice}, 2018);
    std::vector<Team> teams = {team, team2};

saveToFile("test_teams.bin", teams);
    auto loadedTeams = loadFromFile<std::vector<Team>>("test_teams.bin");
    assert(teams == loadedTeams);

    std::cout << "Nested structs test passed!\n";
}

void testTypedefAndUsing() {
    std::cout << "Testing typedef and using declarations...\n";

    // Test using declarations (type aliases)
    PersonId id = 12345;
    PersonName name = "John Doe";
    ScoreType score = 95.5f;

    // Test serialization of aliased types
    auto idBuffer = serialize(id);
    auto nameBuffer = serialize(name);
    auto scoreBuffer = serialize(score);

    // Test deserialization
    auto idResult = deserialize<PersonId>(idBuffer);
    auto nameResult = deserialize<PersonName>(nameBuffer);
    auto scoreResult = deserialize<ScoreType>(scoreBuffer);

    assert(id == idResult);
    assert(name == nameResult);
    assert(score == scoreResult);

    // Test typedef types
    IntList intList = {1, 2, 3, 4, 5};
    auto intListBuffer = serialize(intList);
    auto intListResult = deserialize<IntList>(intListBuffer);
    assert(intList == intListResult);

    // Test file I/O with typedef types
saveToFile("test_typedef.bin", intList);
    auto loadedIntList = loadFromFile<IntList>("test_typedef.bin");
    assert(intList == loadedIntList);

    std::cout << "Typedef and using declarations test passed!\n";
}

void testEmployeeStruct() {
    std::cout << "Testing Employee struct with typedef types...\n";

    // Create test employee
    Employee emp1(1001, "Alice Johnson", 92.5f, {101, 102, 103});
    Employee emp2(1002, "Bob Smith", 88.0f, {201, 202});

    // Test single employee
    auto empBuffer = serialize(emp1);
    auto empResult = deserialize<Employee>(empBuffer);
    assert(emp1 == empResult);

    // Test vector of employees
    std::vector<Employee> employees = {emp1, emp2};
    auto employeesBuffer = serialize(employees);
    auto employeesResult = deserialize<std::vector<Employee>>(employeesBuffer);
    assert(employees == employeesResult);

    // Test file I/O
saveToFile("test_employee.bin", emp1);
saveToFile("test_employees.bin", employees);

    auto loadedEmp = loadFromFile<Employee>("test_employee.bin");
    auto loadedEmployees = loadFromFile<std::vector<Employee>>("test_employees.bin");

    assert(emp1 == loadedEmp);
    assert(employees == loadedEmployees);

    std::cout << "Employee struct test passed!\n";
}

void testVectorsOfCustomTypes() {
    std::cout << "Testing vectors of custom data types...\n";

    // Create test data using typedef vectors
    Person person1(25, 175.5f, "Alice", {1, 3, 7});
    Person person2(30, 180.0f, "Bob", {2, 4, 8});
    PersonList people = {person1, person2};  // typedef std::vector<Person>

    Team team1("Engineering", people, 2020);
    Team team2("Marketing", {person1}, 2018);
    TeamList teams = {team1, team2};  // typedef std::vector<Team>

    // Test PersonList (typedef vector<Person>)
    auto peopleBuffer = serialize(people);
    auto peopleResult = deserialize<PersonList>(peopleBuffer);
    assert(people == peopleResult);

    // Test TeamList (typedef vector<Team>)
    auto teamsBuffer = serialize(teams);
    auto teamsResult = deserialize<TeamList>(teamsBuffer);
    assert(teams == teamsResult);

    // Test file I/O with typedef vectors
saveToFile("test_person_list.bin", people);
saveToFile("test_team_list.bin", teams);

    auto loadedPeople = loadFromFile<PersonList>("test_person_list.bin");
    auto loadedTeams = loadFromFile<TeamList>("test_team_list.bin");

    assert(people == loadedPeople);
    assert(teams == loadedTeams);

    std::cout << "Vectors of custom data types test passed!\n";
}

void testComplexDepartmentStruct() {
    std::cout << "Testing complex Department struct...\n";

    // Create complex nested data structure
    Person alice(25, 175.5f, "Alice", {1, 3, 7});
    Person bob(30, 180.0f, "Bob", {2, 4, 8});
    Person charlie(28, 170.0f, "Charlie", {5, 9});

    PersonList people = {alice, bob, charlie};

    Team devTeam("Development", {alice, bob}, 2019);
    Team qaTeam("QA", {charlie}, 2020);
    TeamList teams = {devTeam, qaTeam};

    Employee emp1(1001, "Alice Manager", 95.0f, {101, 102});
    Employee emp2(1002, "Bob Lead", 92.0f, {201, 202, 203});
    std::vector<Employee> employees = {emp1, emp2};

    IntList budget = {100000, 150000, 80000};

    Department dept("Technology", people, teams, employees, budget);

    // Test serialization/deserialization
    auto deptBuffer = serialize(dept);
    auto deptResult = deserialize<Department>(deptBuffer);
    assert(dept == deptResult);

    // Test file I/O
saveToFile("test_department.bin", dept);
    auto loadedDept = loadFromFile<Department>("test_department.bin");
    assert(dept == loadedDept);

    // Test vector of departments
    Department dept2("Sales", {alice}, {}, {emp1}, {50000});
    std::vector<Department> company = {dept, dept2};

saveToFile("test_company.bin", company);
    auto loadedCompany = loadFromFile<std::vector<Department>>("test_company.bin");
    assert(company == loadedCompany);

    std::cout << "Complex Department struct test passed!\n";
}

void testUtilityFunctions() {
    std::cout << "Testing utility functions...\n";

    // Create a test file
    std::string testData = "Test file content";
saveToFile("utility_test.bin", testData);

    // Test file existence
    assert(fileExists("utility_test.bin"));
    assert(!fileExists("nonexistent.bin"));

    // Test file size
    auto buffer = serialize(testData);
    std::size_t expectedSize = buffer.size();
    std::size_t actualSize = getFileSize("utility_test.bin");
    assert(expectedSize == actualSize);

    std::cout << "Utility functions test passed!\n";
}

void cleanupTestFiles() {
    // Clean up test files (basic implementation)
    std::vector<std::string> testFiles = {
        "test_int.bin", "test_float.bin", "test_string.bin",
        "test_int_vector.bin", "test_string_vector.bin",
        "test_person.bin", "test_people.bin", "test_team.bin",
        "test_teams.bin", "utility_test.bin", "test_typedef.bin",
        "test_employee.bin", "test_employees.bin", "test_person_list.bin",
        "test_team_list.bin", "test_department.bin", "test_company.bin"
    };

    for (const auto& file : testFiles) {
        std::remove(file.c_str());
    }
}

int main() {
    std::cout << "=== Utils Serde Library Test Suite ===\n\n";

    try {
        testPodTypes();
        testStrings();
        testVectors();
        testUserDefinedStructs();
        testNestedStructs();
        testTypedefAndUsing();
        testEmployeeStruct();
        testVectorsOfCustomTypes();
        testComplexDepartmentStruct();
        testUtilityFunctions();

        std::cout << "\n=== All tests passed! ===\n";

        // Demonstrate advanced usage
        std::cout << "\n=== Advanced Usage Demo ===\n";

        // Create complex nested data
        Person dev1(28, 170.0f, "Charlie", {13, 17, 19});
        Person dev2(32, 185.0f, "Diana", {23, 29, 31});
        Team devTeam("Development", {dev1, dev2}, 2019);

        std::vector<Team> company = {devTeam};

        // Save to file
saveToFile("company_data.bin", company);
        std::cout << "Saved company data to file (size: "
                  << getFileSize("company_data.bin") << " bytes)\n";

        // Load from file
        auto loadedCompany = loadFromFile<std::vector<Team>>("company_data.bin");
        std::cout << "Loaded company with " << loadedCompany.size() << " teams\n";
        std::cout << "First team: " << loadedCompany[0].mTeamName
                  << " with " << loadedCompany[0].mMembers.size() << " members\n";

        // Clean up
        cleanupTestFiles();
        std::remove("company_data.bin");

    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        cleanupTestFiles();
        return 1;
    }

    return 0;
}
