#include "stdafx.h"
#include "Node.h"
#include <iostream>

bool Node::evaluate() {

	if (priority < 10) return found;
	if (priority == 10) {

		for (unsigned int i = 0; i < children.size(); ++i) {
			if (children[i]->evaluate() == false) return false;
		}
		return true;
	}
	if (priority == 11) {


		for (unsigned int i = 0; i < children.size(); ++i) {

			if (children[i]->evaluate()) return true;
		}
		return false;
	}
	std::wcout << "Undefined Node priority\n";
	return false;
}
