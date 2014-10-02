# LogicalGui [![Build Status](https://travis-ci.org/02JanDal/LogicalGui.svg?branch=master)](https://travis-ci.org/02JanDal/LogicalGui)

LogicalGui allows you to cleanly separate logic and non-logic (GUI) code. This is done by giving callbacks a string as identifier, and binding functions, slots, lambdas etc. to these string IDs.

This also allows for easier unit testing, without depending on the GUI.

### Features

* String based callback look-up
* Automatic thread handling
    * If the receiver is in a different thread the callback will be called in the thread of the receiver, and the calling thread will wait.
* C++11 variadic templates for nice syntax

### Use cases

* Separating GUI code from logic code
* Getting rid of signal-spaghetti
* Cleaner unit tests that don't depend on GUI code
    * You can just bind a callback ID to a dummy method that returns test data

### Dependencies and Requirements

* Qt 5 ([Download](https://qt-project.org/downloads))
* C++11 compiler (GCC or Clang recommended)

### Example usage

```c++
class MyClass : public Bindable
{
public:
	void doSomething()
	{
		// stuff here
		const QString result = wait<QString>("AskUserSomething", "This is a message to the user");
		// some stuff with the result here
	}
};

int main(int argc, char **argv)
{
	MyClass obj;
	obj.bind("AskUserSomething", [](const QString &msg) {
		// show a dialog to the user, asking for input
		// return the result
	});
	obj.doSomething();
}
```

## Contributing

Normal procedure; create a fork, edit code, submit PR. Licensed under Apache 2.0. I use clang-format for code formating, either use it, or refer to [.clang-format](https://github.com/02JanDal/LogicalGui/blob/master/.clang-format) for the code style used.