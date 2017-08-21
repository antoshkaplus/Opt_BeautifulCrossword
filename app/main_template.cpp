
#include "beauty_crossword.hpp"


int main() {
    int N;
    int W;
    cin >> N;
    cin >> W;
    vector<string> words(W);
    for (auto& w : words) {
        cin >> w;
    }   
    vector<int> weights(4);
    for (auto& w : weights) {
        cin >> w;
    }
    
    BeautifulCrossword bc;
    auto res = bc.generateCrossword(N, words, weights);
    
    for (auto& r : res) {
        cout << r << endl;
    }
}
