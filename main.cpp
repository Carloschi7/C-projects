#include <iostream>
#include "Encrypter.h"


int main()
{
	Unsafe::EncryptionKey key = Unsafe::EncryptionKey::GenKey(69);

	Unsafe::Encrypter encrypter("src/prova.txt", key);
	encrypter.Decrypt();

	return 0;
}