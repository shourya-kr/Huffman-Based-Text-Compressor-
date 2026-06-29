#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
#include <bitset>
using namespace std;

//NODE STRUCTURE
struct Node {
    char ch;
    int freq;
    Node* left;
    Node* right;

    Node(char c, int f) {
        ch = c;
        freq = f;
        left = nullptr;
        right = nullptr;
    }
};

// Min-heap comparator — lowest frequency on top
struct Compare {
    bool operator()(Node* a, Node* b) {
        return a->freq > b->freq;
    }
};

//COUNT FREQUENCIES
unordered_map<char, int> buildFrequencyMap(const string& text) {
    unordered_map<char, int> freqMap;
    for (char ch : text) {
        freqMap[ch]++;
    }
    return freqMap;
}

//BUILD MIN-HEAP
priority_queue<Node*, vector<Node*>, Compare> buildMinHeap(unordered_map<char, int>& freqMap) {
    priority_queue<Node*, vector<Node*>, Compare> minHeap;
    for (auto& pair : freqMap) {
        minHeap.push(new Node(pair.first, pair.second));
    }
    return minHeap;
}

//BUILD HUFFMAN TREE
Node* buildHuffmanTree(priority_queue<Node*, vector<Node*>, Compare> minHeap) {
    while (minHeap.size() > 1) {
        Node* left  = minHeap.top(); minHeap.pop();
        Node* right = minHeap.top(); minHeap.pop();

        Node* merged = new Node('$', left->freq + right->freq);
        merged->left  = left;
        merged->right = right;

        minHeap.push(merged);
    }
    return minHeap.top();
}

//GENERATE HUFFMAN CODES
void generateCodes(Node* root, string code, unordered_map<char, string>& huffmanCodes) {
    if (!root) return;

    if (!root->left && !root->right) {
        huffmanCodes[root->ch] = code;
        return;
    }

    generateCodes(root->left,  code + "0", huffmanCodes);
    generateCodes(root->right, code + "1", huffmanCodes);
}

//COMPRESS AND WRITE TO FILE
void writeCompressedFile(const string& inputFile,
                          const string& outputFile,
                          unordered_map<char, string>& huffmanCodes,
                          unordered_map<char, int>& freqMap) {

    ifstream inFile(inputFile);
    ofstream outFile(outputFile, ios::binary);

    if (!inFile || !outFile) {
        cout << "Error opening files!\n";
        return;
    }

    string text((istreambuf_iterator<char>(inFile)),
                  istreambuf_iterator<char>());
    inFile.close();

    // Build full encoded bit string
    string encodedStr = "";
    for (char ch : text) encodedStr += huffmanCodes[ch];

    // Write original character count
    int originalSize = text.size();
    outFile.write(reinterpret_cast<char*>(&originalSize), sizeof(int));

    // Write frequency map size
    int mapSize = freqMap.size();
    outFile.write(reinterpret_cast<char*>(&mapSize), sizeof(int));

    // Write each character + frequency (to rebuild tree when decompressing)
    for (auto& pair : freqMap) {
        outFile.write(&pair.first, 1);
        outFile.write(reinterpret_cast<const char*>(&pair.second), sizeof(int));
    }

    // Calculate and write padding
    int padding = 8 - (encodedStr.size() % 8);
    if (padding == 8) padding = 0;
    outFile.write(reinterpret_cast<char*>(&padding), sizeof(int));

    // Add padding zeros
    for (int i = 0; i < padding; i++) encodedStr += '0';

    // Pack bits into bytes and write
    for (int i = 0; i < (int)encodedStr.size(); i += 8) {
        bitset<8> byte(encodedStr.substr(i, 8));
        unsigned char c = (unsigned char)byte.to_ulong();
        outFile.write(reinterpret_cast<char*>(&c), 1);
    }

    outFile.close();

    cout << "File compressed successfully!\n";
    cout << "Original size:   " << originalSize << " bytes\n";
    cout << "Compressed size: " << (encodedStr.size() / 8) << " bytes\n";
    cout << "Compression ratio: "
         << (1.0 - (encodedStr.size() / 8.0) / originalSize) * 100
         << "%\n";
}

//DECOMPRESS
void decompressFile(const string& compressedFile,
                     const string& outputFile) {

    ifstream inFile(compressedFile, ios::binary);
    ofstream outFile(outputFile);

    if (!inFile || !outFile) {
        cout << "Error opening files!\n";
        return;
    }

    // Read original size
    int originalSize;
    inFile.read(reinterpret_cast<char*>(&originalSize), sizeof(int));

    // Read frequency map size
    int mapSize;
    inFile.read(reinterpret_cast<char*>(&mapSize), sizeof(int));

    // Rebuild frequency map
    unordered_map<char, int> freqMap;
    for (int i = 0; i < mapSize; i++) {
        char ch;
        int freq;
        inFile.read(&ch, 1);
        inFile.read(reinterpret_cast<char*>(&freq), sizeof(int));
        freqMap[ch] = freq;
    }

    // Read padding
    int padding;
    inFile.read(reinterpret_cast<char*>(&padding), sizeof(int));

    // Read compressed bytes and convert back to bit string
    string encodedStr = "";
    unsigned char byte;
    while (inFile.read(reinterpret_cast<char*>(&byte), 1)) {
        bitset<8> bits(byte);
        encodedStr += bits.to_string();
    }
    inFile.close();

    // Remove padding bits from end
    encodedStr = encodedStr.substr(0, encodedStr.size() - padding);

    // Rebuild Huffman tree from frequency map
    auto minHeap = buildMinHeap(freqMap);
    Node* root   = buildHuffmanTree(minHeap);

    // Walk tree using bit string to decode characters
    string decodedText = "";
    Node* curr = root;
    for (char bit : encodedStr) {
    if ((int)decodedText.size() == originalSize) break; // stop here
    
    if (bit == '0') curr = curr->left;
    else            curr = curr->right;

    if (!curr->left && !curr->right) {
        decodedText += curr->ch;
        curr = root;
    }
}

    outFile << decodedText;
    outFile.close();

    cout << "File decompressed successfully!\n";
    cout << "Characters recovered: " << decodedText.size() << "\n";
}

//MAIN
int main() {
    string inputFile        = "test.txt";
    string compressedFile   = "compressed.bin";
    string decompressedFile = "decompressed.txt";

    // Read input file
    ifstream f(inputFile);
    if (!f) {
        cout << "Could not open test.txt — please create it first!\n";
        return 1;
    }
    string text((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    f.close();

    // Build frequency map
    auto freqMap = buildFrequencyMap(text);

    // Build min-heap and Huffman tree
    auto minHeap = buildMinHeap(freqMap);
    Node* root   = buildHuffmanTree(minHeap);

    // Generate codes
    unordered_map<char, string> huffmanCodes;
    generateCodes(root, "", huffmanCodes);

    // Print codes
    cout << "Huffman Codes:\n";
    for (auto& pair : huffmanCodes) {
        cout << "  '" << pair.first << "' : " << pair.second << "\n";
    }

    // Compress
    cout << "\nCOMPRESSING\n";
    writeCompressedFile(inputFile, compressedFile, huffmanCodes, freqMap);

    // Decompress
    cout << "\nDECOMPRESSING\n";
    decompressFile(compressedFile, decompressedFile);

    // Verify
    ifstream orig(inputFile);
    ifstream decomp(decompressedFile);
    string origText((istreambuf_iterator<char>(orig)),     istreambuf_iterator<char>());
    string decompText((istreambuf_iterator<char>(decomp)), istreambuf_iterator<char>());

    cout << "\nVERIFICATION\n";
    if (origText == decompText)
        cout << "SUCCESS — Decompressed file matches original perfectly!\n";
    else
        cout << "MISMATCH — Something went wrong.\n";

    return 0;
}
