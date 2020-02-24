// See README.txt for information and build instructions.

#include <iostream>
#include <fstream>
#include <string>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/type_resolver.h>
#include <google/protobuf/util/type_resolver_util.h>
//#include <google/protobuf/util/json_format_proto3.pb.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "addressbook.pb.h"
using namespace google;
using namespace protobuf;
using namespace util;
using namespace std;

// Iterates though all people in the AddressBook and prints info about them.
void ListPeople(const tutorial::AddressBook& address_book) {
  for (int i = 0; i < address_book.people_size(); i++) {
    const tutorial::Person& person = address_book.people(i);

    cout << "Person ID: " << person.id() << endl;
    cout << "  Name: " << person.name() << endl;
    if (person.email() != "") {
      cout << "  E-mail address: " << person.email() << endl;
    }

    for (int j = 0; j < person.phones_size(); j++) {
      const tutorial::Person::PhoneNumber& phone_number = person.phones(j);

      switch (phone_number.type()) {
        case tutorial::Person::MOBILE:
          cout << "  Mobile phone #: ";
          break;
        case tutorial::Person::HOME:
          cout << "  Home phone #: ";
          break;
        case tutorial::Person::WORK:
          cout << "  Work phone #: ";
          break;
      }
      cout << phone_number.number() << endl;
    }
  }
}

static const char kTypeUrlPrefix[] = "type.googleapis.com";
static std::string GetTypeUrl(const Descriptor* message) {
   return string(kTypeUrlPrefix) + "/" + message->full_name();
}   

// Main function:  Reads the entire address book from a file and prints all
//   the information inside.
int main(int argc, char* argv[]) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (argc != 2) {
    cerr << "Usage:  " << argv[0] << " ADDRESS_BOOK_FILE" << endl;
    return -1;
  }

  tutorial::AddressBook address_book;

  {
    // Read the existing address book.
    fstream input(argv[1], ios::in | ios::binary);
    if (!address_book.ParseFromIstream(&input)) {
      cerr << "Failed to parse address book." << endl;
      return -1;
    }
  }

  ListPeople(address_book);

  google::protobuf::scoped_ptr<TypeResolver> resolver_;
  resolver_.reset(NewTypeResolverForDescriptorPool(kTypeUrlPrefix, DescriptorPool::generated_pool()));
  string result = "";
  JsonOptions options;
  //options.add_whitespace = true;
  GOOGLE_CHECK_OK(BinaryToJsonString(resolver_.get(), GetTypeUrl(address_book.GetDescriptor()), address_book.SerializeAsString(), &result, options));
  std::cout<<result<<endl;

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}
