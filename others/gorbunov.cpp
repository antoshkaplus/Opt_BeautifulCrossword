//#define BeautifulCrossword crossword
//#define generateCrossword generate
#include "vector"
#include "string"
#include "algorithm"
#include "iostream"
#include "functional"
#include "ctime"

using namespace std;

int di[] = { 0, 1, 0, -1 };
int dj[] = { 1, 0, -1, 0 };

const int INF = 1000000000;

class crossword {
private:

    // Result that will be an answer.
    vector<string> result;

    // A list of words that can be filled on the board.
    vector<string> words;

    // used[i] = if a word with index i in words is used.
    vector<int> used;

    // Which word is placed in current cell. -1 if 2 words are placed,
    // -2 if no word is placed.
    vector<vector<int> > which_word;

    // Direction in which given word is placed.
    vector<int> direction;

    // If given word is crossed.
    vector<int> crossed;

    // Coordinates of a given word. (-1, -1) if it is not placed.
    vector<pair<int, int> > coordinates;

    // lengths[i] = a list of words of length i.
    vector<int> lengths[100];

    // by_letter[i][j][k] = a list of indexes in words
    // of such a words that have a letter i on a position j
    // and a word length is k.
    // TODO use linked list.
    vector<int> by_letter[26][100][100];

    // Letter probability.
    double prob[26];

    // Probability that a pair of letters is good for placing
    // into a corner.
    double seq_prob[4][26][26];

    bool vowel[26];

    // Weights of different kinds of beauty.
    vector<int> weights;

    // Maximal length of a word.
    int L;

    // Amount of words.
    int W;

    // Size of the board.
    int N;

    int total_used;

    void put_word(int i, int j, int dir, int w, bool replace = 0) {
        cerr << "Putting word \"" << words[w] << "\" (" << w << ") into position (" <<
             i << ", " << j << ") " << (dir ? "vertically" : "horizontally") << "\n";

        if (!replace)
            total_used += coordinates[w].first == -1;
        coordinates[w] = pair<int, int>(i, j);
        direction[w] = dir;

        for (int k = 0; k < (int)words[w].size(); k++) {
            result[i + di[dir] * k][j + dj[dir] * k] = words[w][k];

            if (which_word[i + di[dir] * k][j + dj[dir] * k] == -2)
                which_word[i + di[dir] * k][j + dj[dir] * k] = w;
            else {
                if (which_word[i + di[dir] * k][j + dj[dir] * k] >= 0)
                    crossed[which_word[i + di[dir] * k][j + dj[dir] * k]] = 1;
                crossed[w] = 1;
                which_word[i + di[dir] * k][j + dj[dir] * k] = -1;
            }
        }

        used[w] = 1;
    }

    void unput_word(int w) {
        cerr << "Unputting word " << words[w] << "\n";
        //cerr << coordinates[w].first << " " << coordinates[w].second << "\n";
        int i = coordinates[w].first;
        int j = coordinates[w].second;
        int dir = direction[w];
        for (int k = 0; k < (int)words[w].size(); k++) {
            which_word[i][j] = -2;
            i += di[dir];
            j += dj[dir];
        }
        used[w] = 0;
    }

    void put_word_square(int i, int j, vector<int> &use) {
        int len = words[use[0]].size();
        //cerr << "Putting complete square\n";
        put_word(i, j, 0, use[0]);
        put_word(i, j, 1, use[1]);
        put_word(i, j + len - 1, 1, use[2]);
        put_word(i + len - 1, j, 0, use[3]);
    }

    void put_word_octet(int first_row, int shift, vector<int> &use) {
        int len = (int)words[use[0]].size();

        cerr << "Puting octet\n";

        // Top left top.
        put_word(first_row, first_row + shift, 0, use[0]);
        // Top left left.
        put_word(first_row + shift, first_row, 1, use[1]);

        // Top right top.
        put_word(first_row, (N - 1) - first_row - (len - 1) - shift, 0, use[2]);
        // Top right right.
        put_word(first_row + shift, (N - 1) - first_row, 1, use[3]);

        // Bottom left left.
        put_word((N - 1) - first_row - (len - 1) - shift, first_row, 1, use[4]);
        // Bottom left bottom.
        put_word((N - 1) - first_row, first_row + shift, 0, use[5]);

        // Bottom right right.
        put_word((N - 1) - first_row - (len - 1) - shift, (N - 1) - first_row, 1, use[6]);
        // Bottom right bottom.
        put_word((N - 1) - first_row, (N - 1) - first_row - (len - 1) - shift, 0, use[7]);

        for (int i = 0; i < 8; i++)
            used[use[i]] = 1;
    }

    void put_word_quartet(int first_row, int shift, vector<int> &use) {
        int len = (int)words[use[0]].size();

        //cerr << "Putting quartet\n";

        // Top.
        put_word(first_row, first_row + shift, 0, use[0]);

        // Left.
        put_word(first_row + shift, first_row, 1, use[1]);

        // Right.
        put_word(first_row + shift, (N - 1) - first_row, 1, use[2]);

        // Bottom.
        put_word((N - 1) - first_row, first_row + shift, 0, use[3]);

        for (int i = 0; i < 4; i++)
            used[use[i]] = 1;
    }

    bool add_similar(int len, int match0, int match1, vector<int> &use) {
        for (int letter = 0; letter < 26; letter++) {
            //cerr << "Sizes of chunks for letter " << (char)(letter + 'A') << " [" <<
            //	match0 << "][" << match1 << "][" << len << "] = " <<
            //	by_letter[letter][match0][len].size() << ", " <<
            //	by_letter[letter][match1][len].size() << "\n";

            for (int i = 0; i < (int)by_letter[letter][match0][len].size(); i++)
                for (int j = 0; j < (int)by_letter[letter][match1][len].size(); j++)
                    if (
                        !used[by_letter[letter][match0][len][i]] &&
                        !used[by_letter[letter][match1][len][j]] &&
                        by_letter[letter][match0][len][i] !=
                        by_letter[letter][match1][len][j]
                        ) {
                        use.push_back(by_letter[letter][match0][len][i]);
                        used[use.back()] = 1;

                        use.push_back(by_letter[letter][match1][len][j]);
                        used[use.back()] = 1;

                        return 1;
                    }
        }
        return 0;
    }

    int generate_single_square(int first_row) {
        vector<int> use;
        int len = N - first_row * 2;
        for (int i = 0; i < (int)lengths[len].size(); i++) {
            int word0 = lengths[len][i];
            if (used[word0])
                continue;
            int letter0 = words[word0][0] - 'A';
            int letter1 = words[word0][len - 1] - 'A';
            for (int j = 0; j < (int)by_letter[letter0][0][len].size(); j++) {
                int word1 = by_letter[letter0][0][len][j];
                if (used[word1] || word1 == word0)
                    continue;
                int letter2 = words[word1][len - 1] - 'A';
                for (int k = 0; k < (int)by_letter[letter1][0][len].size(); k++) {
                    int word2 = by_letter[letter1][0][len][k];
                    if (used[word2] || word2 == word1 || word2 == word0)
                        continue;
                    int letter3 = words[word2][len - 1] - 'A';
                    for (int l = 0; l < (int)by_letter[letter2][0][len].size(); l++) {
                        int word3 = by_letter[letter2][0][len][l];
                        if (used[word3] || word3 == word2 || word3 == word1 || word3 == word0 ||
                            letter3 != words[word3][len - 1] - 'A')
                            continue;
                        use.push_back(word0);
                        use.push_back(word1);
                        use.push_back(word2);
                        use.push_back(word3);
                        cerr << "Found square\n";
                        put_word_quartet(first_row, 0, use);
                        return len;
                    }
                }
            }
        }
        return 0;
    }

    bool possible_corners(int first_row, int len, vector<int> &use) {
        if ((first_row + len) * 2 >= N)
            return 0;

        cerr << "In possible_corners(" << first_row << ", " <<
             len << ")\n";

        use.clear();

        // These calls change used[] state of words.
        add_similar(len, 0, 0, use);
        add_similar(len, len - 1, 0, use);
        add_similar(len, len - 1, 0, use);
        add_similar(len, len - 1, len - 1, use);

        //cerr << "Found " << use.size() << " words for corners of length " << len << "\n";
        //for (int i = 0; i < (int)use.size(); i++)
        //	cerr << "\"" << words[use[i]] << "\"\n";

        // Restore used[] state of words.
        for (int i = 0; i < (int)use.size(); i++)
            used[use[i]] = 0;

        return use.size() == 8;
    }

    bool possible_corner(
        int first_row, int len, int letter0, int letter1, int pos0, int pos1,
        int match0, int match1, vector<int> &use
    ) {
        cerr << "Searching for a " << len << " letter word with " << pos0 <<
             "th letter " << (char)(letter0 + 'A') << " to match with " <<
             " word with " << pos1 << "th letter " << (char)(letter1 + 'A') <<
             " on positions " << match0 << " " << match1 << "\n";
        use.clear();
        for (int i = 0; i < (int)by_letter[letter0][pos0][len].size(); i++)
            if (!used[by_letter[letter0][pos0][len][i]]) {
                //cerr << words[by_letter[letter0][pos0][len][i]] << ": " <<
                //	by_letter[letter1][pos1][len].size() << "\n";
                for (int j = 0; j < (int)by_letter[letter1][pos1][len].size(); j++)
                    if (
                        !used[by_letter[letter1][pos1][len][j]] &&
                        by_letter[letter0][pos0][len][i] !=
                        by_letter[letter1][pos1][len][j] &&
                        words[by_letter[letter0][pos0][len][i]][match0] ==
                        words[by_letter[letter1][pos1][len][j]][match1]
                        ) {
                        use.resize(2);
                        use[0] = by_letter[letter0][pos0][len][i];
                        use[1] = by_letter[letter1][pos1][len][j];
                        used[use[0]] = used[use[1]] = 1;

                        cerr << "Found " << words[use[0]] << " " << words[use[1]] << "\n";
                        return 1;
                    }
            }
        return 0;
    }

    bool generate_composite_square(int first_row) {
        // Try to create a single square.
        int square = generate_single_square(first_row);
        if (square)
            return 1;

        // Count of cells to fill.
        int K = N - first_row * 2;

        if (K < 3)
            return 0;
        vector<vector<int> > bgaps(2, vector<int>(K, INF));
        vector<vector<int> > blast(2, vector<int>(K, 0));
        vector<vector<int> > btwo(2, vector<int>(K, 0));
        vector<int> bbeginning;
        int blen = L + 1;
        int bgap = INF;
        int bstart = 0;
        int bmiddle = 0;

        // Try all len-gths of the beginnings.
        for (int len = L; len >= 0; len--) {

            // gaps[i][j] = minimum amount of gaps that is possible when the
            // last occupied cell is j. Flag i shows if the middle part is used.
            vector<vector<int> > gaps(2, vector<int>(K, INF));
            // last[i][j] = last length of word for gap[i][j].
            vector<vector<int> > last(2, vector<int>(K, 0));
            // two[i][j] = if there are two consecutive segments of the same length
            // in gaps[i][j].
            vector<vector<int> > two(2, vector<int>(K, 0));

            // Groups of 8 that can be placed.
            vector<int> octets;
            // Groups of 4 that can be placed in the middle.
            vector<int> quartets;


            // Find possible beginning.
            vector<int> beginning;
            if (2 * len + 1 <= K && possible_corners(first_row, len, beginning)) {
                gaps[0][2 * len] = 1;
                last[0][2 * len] = len;
                two[0][2 * len] = 1;
            } else {
                beginning.clear();
                gaps[0][0] = 1;
                last[0][0] = 0;
            }

            for (int k = L; k >= 3; k--) {
                int count = 0;
                for (int i = 0; i < (int)lengths[k].size(); i++)
                    count += !used[lengths[k][i]];
                if (k == len)
                    // Remove 0 or 8 from count.
                    count -= (int)beginning.size();

                while (count >= 8) {
                    octets.push_back(k);
                    count -= 8;
                }
                if (count >= 4)
                    quartets.push_back(k);
            }

            // Put quartets in the middle.
            for (int i = 0; i < (int)quartets.size(); i++)
                for (int j = K - 1; j >= 0; j--)
                    if (gaps[0][j] < INF && quartets[i] % 2 == N % 2) {
                        int m = j + quartets[i] + 1;
                        if (m < K && gaps[1][m] == INF) {
                            gaps[1][m] = gaps[0][j] + 1;
                            last[1][m] = quartets[i];
                            two[1][m] = 0;
                        }
                    }
            // Put octets.
            // Avoid putting single octet twice.
            for (int i = 0; i < (int)octets.size(); i++) {
                for (int k = 1; k >= 0; k--)
                    for (int j = K - 1; j >= 0; j--)
                        if (gaps[k][j] < INF) {
                            // Put it as octet.
                            {
                                int m = j + 2 * octets[i] + 2;
                                if (m < K && gaps[k][m] > gaps[k][j] + 2) {
                                    gaps[k][m] = gaps[k][j] + 2;
                                    last[k][m] = octets[i];
                                    two[k][m] = 1;
                                }
                            }
                            // Put it as quartet.
                            if (k == 0 && octets[i] % 2 == N % 2) {
                                int m = j + octets[i] + 1;
                                if (m < K && gaps[1][m] > gaps[0][j] + 1) {
                                    gaps[1][m] = gaps[0][j] + 1;
                                    last[1][m] = octets[i];
                                    two[1][m] = 0;
                                }
                            }
                        }
            }

            // Save configuration if it is the best for now.
            int start = K - 1;
            int middle = 0;
            for (int i = K - 1; i >= 0; i--)
                for (int j = 0; j <= 1; j++)
                    if (gaps[j][i] + (K - 1) - i < gaps[middle][start] + (K - 1) - start) {
                        start = i;
                        middle = j;
                    }
            if (
                (bbeginning.empty() && !beginning.empty()) ||
                (
                    bgap > gaps[middle][start] + (K - 1) - start &&
                    (bbeginning.empty() || !beginning.empty())
                )
                ) {
                bgap = gaps[middle][start] + (K - 1) - start;
                bstart = start;
                bmiddle = middle;
                blen = !beginning.empty() ? len : 0;
                bgaps = gaps;
                blast = last;
                btwo = two;
                bbeginning = beginning;
            }
        }

        // Now, reconstruct the optimal solution.
        int shift = 0;
        // Start from the corners to avoid reusing them later.
        if (bbeginning.size() == 8) {
            put_word_octet(first_row, 0, bbeginning);
            shift = blen + 1;
        }
        else
            shift = 1;

        // Reconstruct the rest.
        int k = bstart;
        vector<vector<int> > uses;

        while (bstart >= 2 * (blen ? blen + 1 : 1)) {
            int is_middle = !btwo[bmiddle][bstart];
            int last = blast[bmiddle][bstart];

            vector<int> use;
            for (int i = 0; i < (int)lengths[last].size(); i++)
                if ((int)use.size() < 4 * (2 - is_middle) && !used[lengths[last][i]]) {
                    use.push_back(lengths[last][i]);
                    used[lengths[last][i]] = 1;
                }
            uses.push_back(use);

            bmiddle &= !is_middle;
            bstart -= (last + 1) * (2 - is_middle);
        }

        // Add all, except the middle.
        // The beginning is the last part. Skip it if it exists.
        for (int i = 0; i < (int)uses.size(); i++)
            if (uses[i].size() == 8) {
                put_word_octet(first_row, shift, uses[i]);
                shift += 1 + words[uses[i][0]].size();
            }
        // Add the remaining middle.
        for (int i = 0; i < (int)uses.size(); i++)
            if (uses[i].size() == 4) {
                // Center it.
                shift = (N - (int)words[uses[i][0]].size()) / 2 - first_row;
                put_word_quartet(first_row, shift, uses[i]);
                shift += (int)words[uses[i][0]].size();
            }

        return shift > 1;
    }

    bool generate_cross() {
        // N is odd.

        int len = 0;
        int M = N / 2; // Middle.
        int r = M;
        bool match = 0;
        while (
            r - 1 >= 0 &&
            result[r - 1][M] == '.' &&
            result[r - 1][M - 1] == '.' &&
            result[r - 1][M + 1] == '.'
            ) {
            len++;
            r--;
        }
        if (r - 1 >= 0 && result[r - 1][M] != '.') {
            match = 1;
            len++;
            r--;
        }
        len = 2 * len + 1; // Go to both sides.

        while (len >= 3) {
            for (int i = 0; i < (int)lengths[len].size(); i++) {
                string &s0 = words[lengths[len][i]];
                if (used[lengths[len][i]])
                    continue;
                if (match && (s0[0] != result[r][M] || s0[len - 1] != result[r + len - 1][M]))
                    continue;
                for (int j = 0; j < (int)lengths[len].size(); j++) {
                    string &s1 = words[lengths[len][j]];
                    if (used[lengths[len][j]])
                        continue;
                    if (i == j || (
                        match && (result[M][r] != s1[0] || result[M][r + len - 1] != s1[len - 1]))
                        || s0[len / 2] != s1[len / 2]
                        ) continue;

                    cerr << "Found cross\n";
                    put_word(r, M, 1, lengths[len][i]);
                    put_word(M, r, 0, lengths[len][j]);
                    return 1;
                }
            }
            len -= 2 * (1 + match);
            r += 1 + match;
            match = 0;
        }

        return 0;
    }

    void generate_squares() {
        int i = 0;
        while (i < N / 2 - 1 && generate_composite_square(i))
            i += 2;
        if (N % 2 == 1)
            generate_cross();
    }

    bool possible_squares(int len, vector<vector<int> > &use) {
        use.clear();

        //for (int i = 0; i < (int)lengths[len].size(); i++)
        //	cerr << "\"" << words[lengths[len][i]] << "\"\n";
        bool added = 1;
        while (added && use.size() < 4) {
            added = 0;

            for (int i = 0; i < (int)lengths[len].size(); i++) {
                int word0 = lengths[len][i];
                if (used[word0])
                    continue;
                int letter0 = words[word0][0] - 'A';
                int letter1 = words[word0][len - 1] - 'A';
                for (int j = 0; j < (int)by_letter[letter0][0][len].size(); j++) {
                    int word1 = by_letter[letter0][0][len][j];
                    if (used[word1] || word1 == word0)
                        continue;
                    int letter2 = words[word1][len - 1] - 'A';
                    for (int k = 0; k < (int)by_letter[letter1][0][len].size(); k++) {
                        int word2 = by_letter[letter1][0][len][k];
                        if (used[word2] || word2 == word1 || word2 == word0)
                            continue;
                        int letter3 = words[word2][len - 1] - 'A';
                        for (int l = 0; l < (int)by_letter[letter2][0][len].size(); l++) {
                            int word3 = by_letter[letter2][0][len][l];
                            if (used[word3] || word3 == word2 || word3 == word1 || word3 == word0 ||
                                letter3 != words[word3][len - 1] - 'A')
                                continue;

                            vector<int> now(4);
                            now[0] = word0;
                            now[1] = word1;
                            now[2] = word2;
                            now[3] = word3;

                            use.push_back(now);
                            added = 1;

                            used[word0] = used[word1] = used[word2] = used[word3] = 1;

                            goto exit_loops;
                        }
                    }
                }
            }
            exit_loops: ;
        }

        // Clear temporary data.
        for (int i = 0; i < (int)use.size(); i++)
            for (int j = 0; j < (int)use[i].size(); j++)
                used[use[i][j]] = 0;

        return !use.empty();
    }

    int search_match(int len, int letter0, int letter1) {
        //cerr << "Searching for word of length " << len <<
        //	" with first letter " << (char)(letter0 + 'A') <<
        //	" and last letter " << (char)(letter1 + 'A') << " for placing in the solid middle.\n";
        for (int i = 0; i < (int)by_letter[letter0][0][len].size(); i++)
            if (
                !used[by_letter[letter0][0][len][i]] &&
                words[by_letter[letter0][0][len][i]][len - 1] - 'A' == letter1
                ) {
                cerr << "Found middle \"" << words[by_letter[letter0][0][len][i]] << "\"\n";
                used[by_letter[letter0][0][len][i]] = 1;
                return by_letter[letter0][0][len][i];
            }
        return -1;
    }
    bool search_any(int len, int count, vector<int> &use) {
        for (int i = 0; i < (int)lengths[len].size() && (int)use.size() < count; i++)
            if (!used[lengths[len][i]])
                use.push_back(lengths[len][i]);
        return (int)use.size() == len;
    }
    int search_position_match(int len, int pos, int letter) {
        cerr << "Searching for word of length " << len <<
             " with " << pos << "th letter " << (char)(letter + 'A') << "\n";
        for (int i = 0; i < (int)by_letter[letter][pos][len].size(); i++)
            if (!used[by_letter[letter][pos][len][i]]) {
                used[by_letter[letter][pos][len][i]] = 1;
                cerr << "Found " << words[by_letter[letter][pos][len][i]] << "\n";
                return by_letter[letter][pos][len][i];
            }
        return -1;
    }

    bool generate_solid_middle(int first_row) {
        vector<int> matches(4, -1);
        int len = N - 12 * 2 + 2;
        //cerr << "Trying to generate solid middle\n";

        matches[0] = search_match(len,
                                  result[first_row][12 - 1] - 'A',
                                  result[first_row][(N - 1) - (12 - 1)] - 'A');
        matches[1] = search_match(len,
                                  result[12 - 1][first_row] - 'A',
                                  result[(N - 1) - (12 - 1)][first_row] - 'A');
        matches[2] = search_match(len,
                                  result[12 - 1][(N - 1) - first_row] - 'A',
                                  result[(N - 1) - (12 - 1)][(N - 1) - first_row] - 'A');
        matches[3] = search_match(len,
                                  result[(N - 1) - first_row][12 - 1] - 'A',
                                  result[(N - 1) - first_row][(N - 1) - (12 - 1)] - 'A');

        // Tear down pseudo-used words.
        for (int i = 0; i < 4; i++)
            if (matches[i] == -1) {
                for (int j = 0; j < 4; j++)
                    if (matches[j] != -1)
                        used[matches[j]] = 0;
                return 0;
            }
        put_word(first_row, 12 - 1, 0, matches[0]);
        put_word(12 - 1, first_row, 1, matches[1]);
        put_word(12 - 1, (N - 1) - first_row, 1, matches[2]);
        put_word((N - 1) - first_row, 12 - 1, 0, matches[3]);
        return 1;
    }

    int possible_beginning(int letter, int match, int len) {
        if (letter == (int)'.' - 'A')
            return -1;
        else
            for (int i = 0; i < (int)by_letter[letter][match][len].size(); i++)
                if (!used[by_letter[letter][match][len][i]]) {
                    used[by_letter[letter][match][len][i]] = 1;
                    return by_letter[letter][match][len][i];
                }
        return -1;
    }

    bool possible_beginnings(int i, int j, int len, vector<int> &use) {
        use.resize(8);
        use[0] = possible_beginning(result[i][j] - 'A', 0, len);
        use[1] = possible_beginning(result[j][i] - 'A', 0, len);
        use[2] = possible_beginning(result[i][(N - 1) - j] - 'A', len - 1, len);
        use[3] = possible_beginning(result[j][(N - 1) - i] - 'A', 0, len);
        use[4] = possible_beginning(result[(N - 1) - j][i] - 'A', len - 1, len);
        use[5] = possible_beginning(result[(N - 1) - i][j] - 'A', 0, len);
        use[6] = possible_beginning(result[(N - 1) - j][(N - 1) - i] - 'A', len - 1, len);
        use[7] = possible_beginning(result[(N - 1) - i][(N - 1) - j] - 'A', len - 1, len);

        bool valid = 1;
        for (int k = 0; k < 8; k++)
            if (use[k] == -1)
                valid = 0;
        // Tear down pseudo-used words.
        for (int k = 0; k < 8; k++)
            if (use[k] != -1)
                used[use[k]] = 0;

        //cerr << "Possible beginnings:\n";
        //for (int k = 0; k < 8; k++)
        //	if (use[k] != -1)
        //		cerr << k << ": \"" << words[use[k]] << "\"\n";

        return valid;
    }

    bool generate_middle(int first_row) {
        // Try to fill using single lines.
        if (first_row != 0 && generate_solid_middle(first_row))
            return 1;

        // Count of cells to fill.
        int K = N - 12 * 2 + 2;
        int r = 12 - 1;

        if (K < 3)
            return 0;

        //cerr << "K = " << K << "\n";
        //cerr << "Coordinates: " << first_row << ", " << r << "\n";

        vector<vector<int> > bgaps(2, vector<int>(K, INF));
        vector<vector<int> > blast(2, vector<int>(K, 0));
        vector<vector<int> > btwo(2, vector<int>(K, 0));
        vector<int> bbeginning;
        int blen = L + 1;
        int bgap = INF;
        int bstart = 0;
        int bmiddle = 0;

        // Try all len-gths of the beginnings.
        for (int len = L; len >= 0; len--) {

            // gaps[i][j] = minimum amount of gaps that is possible when the
            // last occupied cell is j. Flag i shows if the middle part is used.
            vector<vector<int> > gaps(2, vector<int>(K, INF));
            // last[i][j] = last length of word for gap[i][j].
            vector<vector<int> > last(2, vector<int>(K, 0));
            // two[i][j] = if there are two consecutive segments of the same length
            // in gaps[i][j].
            vector<vector<int> > two(2, vector<int>(K, 0));

            // Groups of 8 that can be placed.
            vector<int> octets;
            // Groups of 4 that can be placed in the middle.
            vector<int> quartets;


            // Find possible beginning.
            vector<int> beginning;
            if (
                2 * len + 1 <= K && first_row != 0 &&
                possible_beginnings(first_row, r, len, beginning)
                ) {
                gaps[0][2 * len] = 1;
                last[0][2 * len] = len;
                two[0][2 * len] = 1;
            } else {
                beginning.clear();
                // Skip first and last two cells.
                gaps[0][2] = 3;
                last[0][2] = 0;
            }

            for (int k = L; k >= 3; k--) {
                int count = 0;
                for (int i = 0; i < (int)lengths[k].size(); i++)
                    count += !used[lengths[k][i]];
                if (k == len)
                    // Remove 0 or 8 from count.
                    count -= (int)beginning.size();

                while (count >= 8) {
                    octets.push_back(k);
                    count -= 8;
                }
                if (count >= 4)
                    quartets.push_back(k);
            }

            // Put quartets in the middle.
            for (int i = 0; i < (int)quartets.size(); i++)
                for (int j = K - 1; j >= 0; j--)
                    if (gaps[0][j] < INF && quartets[i] % 2 == N % 2) {
                        int m = j + quartets[i] + 1;
                        if (m < K && gaps[1][m] == INF) {
                            gaps[1][m] = gaps[0][j] + 1;
                            last[1][m] = quartets[i];
                            two[1][m] = 0;
                        }
                    }
            // Put octets.
            // Avoid putting single octet twice.
            for (int i = 0; i < (int)octets.size(); i++) {
                for (int k = 1; k >= 0; k--)
                    for (int j = K - 1; j >= 0; j--)
                        if (gaps[k][j] < INF) {
                            // Put it as octet.
                            {
                                int m = j + 2 * octets[i] + 2;
                                if (m < K && gaps[k][m] > gaps[k][j] + 2) {
                                    gaps[k][m] = gaps[k][j] + 2;
                                    last[k][m] = octets[i];
                                    two[k][m] = 1;
                                }
                            }
                            // Put it as quartet.
                            if (k == 0 && octets[i] % 2 == N % 2) {
                                int m = j + octets[i] + 1;
                                if (m < K && gaps[1][m] > gaps[0][j] + 1) {
                                    gaps[1][m] = gaps[0][j] + 1;
                                    last[1][m] = octets[i];
                                    two[1][m] = 0;
                                }
                            }
                        }
            }

            // Save configuration if it is the best for now.
            int start = K - 1;
            int middle = 0;
            for (int i = K - 1; i >= 0; i--)
                for (int j = 0; j <= 1; j++)
                    if (gaps[j][i] + (K - 1) - i < gaps[middle][start] + (K - 1) - start) {
                        start = i;
                        middle = j;
                    }

            //cerr << "For length = " << len << " the best start is " << start <<
            //    " and the middle is " << (middle ? "" : "not ") << "used\n";
            //cerr << "Amount of gaps is " << gaps[middle][start] + (K - 1) - start << "\n";
            //cerr << "Additional gap is " << (K - 1) - start << "\n";
            //cerr << "The last length is " << last[middle][start] << "\n";
            //cerr << "The beginning is ";
            //for (int i = 0; i < (int)beginning.size(); i++)
            //    cerr << "\"" << words[beginning[i]] << "\"" <<
            //		(i == (int)beginning.size() - 1 ? "" : ", ");
            //cerr << "\n";

            // TODO Maybe those lines without beginnings are already in a loosing
            // position?
            if (
                (bbeginning.empty() && !beginning.empty()) ||
                (
                    bgap > gaps[middle][start] + (K - 1) - start &&
                    (bbeginning.empty() || !beginning.empty())
                )
                ) {
                bgap = gaps[middle][start] + (K - 1) - start;
                bstart = start;
                bmiddle = middle;
                blen = !beginning.empty() ? len : 0;
                bgaps = gaps;
                blast = last;
                btwo = two;
                bbeginning = beginning;
            }
        }

        // Now, reconstruct the optimal solution.
        int shift = 13 - first_row;
        // Start from the corners to avoid reusing them later.
        if (bbeginning.size() == 8) {
            shift -= 2;
            put_word_octet(first_row, shift, bbeginning);
            shift += blen + 1;
        }

        // Reconstruct the rest.
        int k = bstart;
        vector<vector<int> > uses;

        while (bstart >= 2 * (blen ? blen + 1 : 2)) {
            int is_middle = !btwo[bmiddle][bstart];
            int last = blast[bmiddle][bstart];

            vector<int> use;
            for (int i = 0; i < (int)lengths[last].size(); i++)
                if ((int)use.size() < 4 * (2 - is_middle) && !used[lengths[last][i]]) {
                    use.push_back(lengths[last][i]);
                    used[lengths[last][i]] = 1;
                }
            uses.push_back(use);

            bmiddle &= !is_middle;
            bstart -= (last + 1) * (2 - is_middle);
        }

        // Add all, except the middle.
        // The beginning is the last part. Skip it if it exists.
        for (int i = 0; i < (int)uses.size(); i++)
            if (uses[i].size() == 8) {
                put_word_octet(first_row, shift, uses[i]);
                shift += 1 + words[uses[i][0]].size();
            }
        // Add the remaining middle.
        for (int i = 0; i < (int)uses.size(); i++)
            if (uses[i].size() == 4) {
                // Center it.
                shift = (N - (int)words[uses[i][0]].size()) / 2 - first_row;
                put_word_quartet(first_row, shift, uses[i]);
                shift += (int)words[uses[i][0]].size();
            }

        return shift > 1;

    }

    bool generate_middle(int first_row, int delta) {
        // Count of cells to fill.
        int K = N - 2 * (first_row + delta) + 2;
        int r = first_row + delta;

        if (K < 3)
            return 0;

        //cerr << "K = " << K << "\n";
        //cerr << "Coordinates: " << first_row << ", " << r << "\n";

        // gaps[i][j] = minimum amount of gaps that is possible when the
        // last occupied cell is j. Flag i shows if the middle part is used.
        vector<vector<int> > gaps(2, vector<int>(K, INF));
        // last[i][j] = last length of word for gap[i][j].
        vector<vector<int> > last(2, vector<int>(K, 0));
        // two[i][j] = if there are two consecutive segments of the same length
        // in gaps[i][j].
        vector<vector<int> > two(2, vector<int>(K, 0));

        // Groups of 8 that can be placed.
        vector<int> octets;
        // Groups of 4 that can be placed in the middle.
        vector<int> quartets;


        // Skip first and last cells.
        gaps[0][0] = 1;
        last[0][0] = 0;

        for (int k = L; k >= 3; k--) {
            int count = 0;
            for (int i = 0; i < (int)lengths[k].size(); i++)
                count += !used[lengths[k][i]];

            while (count >= 8) {
                octets.push_back(k);
                count -= 8;
            }
            if (count >= 4)
                quartets.push_back(k);
        }

        // Put quartets in the middle.
        for (int i = 0; i < (int)quartets.size(); i++)
            for (int j = K - 1; j >= 0; j--)
                if (gaps[0][j] < INF && quartets[i] % 2 == N % 2) {
                    int m = j + quartets[i] + 1;
                    if (m < K && gaps[1][m] == INF) {
                        gaps[1][m] = gaps[0][j] + 1;
                        last[1][m] = quartets[i];
                        two[1][m] = 0;
                    }
                }
        // Put octets.
        // Avoid putting single octet twice.
        for (int i = 0; i < (int)octets.size(); i++) {
            for (int k = 1; k >= 0; k--)
                for (int j = K - 1; j >= 0; j--)
                    if (gaps[k][j] < INF) {
                        // Put it as octet.
                        {
                            int m = j + 2 * octets[i] + 2;
                            if (m < K && gaps[k][m] > gaps[k][j] + 2) {
                                gaps[k][m] = gaps[k][j] + 2;
                                last[k][m] = octets[i];
                                two[k][m] = 1;
                            }
                        }
                        // Put it as quartet.
                        if (k == 0 && octets[i] % 2 == N % 2) {
                            int m = j + octets[i] + 1;
                            if (m < K && gaps[1][m] > gaps[0][j] + 1) {
                                gaps[1][m] = gaps[0][j] + 1;
                                last[1][m] = octets[i];
                                two[1][m] = 0;
                            }
                        }
                    }
        }

        // Save configuration if it is the best for now.
        int start = K - 1;
        int middle = 0;
        for (int i = K - 1; i >= 0; i--)
            for (int j = 0; j <= 1; j++)
                if (gaps[j][i] + (K - 1) - i < gaps[middle][start] + (K - 1) - start) {
                    start = i;
                    middle = j;
                }

        //cerr << "For length = " << len << " the best start is " << start <<
        //    " and the middle is " << (middle ? "" : "not ") << "used\n";
        //cerr << "Amount of gaps is " << gaps[middle][start] + (K - 1) - start << "\n";
        //cerr << "Additional gap is " << (K - 1) - start << "\n";
        //cerr << "The last length is " << last[middle][start] << "\n";
        //cerr << "The beginning is ";
        //for (int i = 0; i < (int)beginning.size(); i++)
        //    cerr << "\"" << words[beginning[i]] << "\"" <<
        //		(i == (int)beginning.size() - 1 ? "" : ", ");
        //cerr << "\n";

        // TODO Maybe those lines without beginnings are already in a loosing
        // position?


        // Now, reconstruct the optimal solution.
        int shift = delta;

        int k = start;
        vector<vector<int> > uses;

        while (start >= 1) {
            int is_middle = !two[middle][start];
            int cur_last = last[middle][start];

            vector<int> use;
            for (int i = 0; i < (int)lengths[cur_last].size(); i++)
                if ((int)use.size() < 4 * (2 - is_middle) && !used[lengths[cur_last][i]]) {
                    use.push_back(lengths[cur_last][i]);
                    used[lengths[cur_last][i]] = 1;
                }
            uses.push_back(use);

            middle &= !is_middle;
            start -= (cur_last + 1) * (2 - is_middle);
        }

        // Add all, except the middle.
        // The beginning is the last part. Skip it if it exists.
        for (int i = 0; i < (int)uses.size(); i++)
            if (uses[i].size() == 8) {
                put_word_octet(first_row, shift, uses[i]);
                shift += 1 + words[uses[i][0]].size();
            }
        // Add the remaining middle.
        for (int i = 0; i < (int)uses.size(); i++)
            if (uses[i].size() == 4) {
                // Center it.
                shift = (N - (int)words[uses[i][0]].size()) / 2 - first_row;
                put_word_quartet(first_row, shift, uses[i]);
                shift += (int)words[uses[i][0]].size();
            }

        return shift > 1;
    }
    void generate_corner_squares() {
        if (12 * 2 + 1 > N)
            return;

        vector<vector<int> > use[12 + 1];
        for (int len = 4; len <= 12; len += 4) {
            possible_squares(len, use[len]);
            if (use[len].size() < 4) {
                cerr << "Found only " << use[len].size() << " squares " <<
                     "of length " << len << "\n";
                return;
            }
            else
                for (int i = 0; i < (int)use[len].size(); i++)
                    for (int j = 0; j < (int)use[len][i].size(); j++)
                        used[use[len][i][j]] = 1;
        }

        // Place all 12 * 12 squares.
        int d = (N - 1) - (12 - 1);
        for (int i = 12, j = 0; i >= 4; i -= 4, j += 2) {
            put_word_square(j, j, use[i][0]);
            put_word_square(j, d + j, use[i][1]);
            put_word_square(d + j, j, use[i][2]);
            put_word_square(d + j, d + j, use[i][3]);
        }

        // Fill the rest of first 12 / 2 levels.
        for (int i = 0; i < 12; i += 2)
            generate_middle(i);

        // Delegate to corner building routine.
        for (int i = 12; i < N / 2; i += 2)
            generate_composite_square(i);

        if (N % 2 == 1)
            generate_cross();
    }

    void generate_crossings(int len = 9) {
        // Generate center.
        int center_len = N % 4;
        int last_row = (N - center_len) / 2;
        while (center_len <= 8 && !generate_single_square(last_row)) {
            last_row -= 2;
            center_len += 4;
        }
        if (center_len > 8)
            last_row = (N - N % 4) / 2;
        else {
            center_len += 4;
            last_row -= 2;
            while (center_len <= N && generate_single_square(last_row)) {
                center_len += 4;
                last_row -= 2;
            }
            if (N % 2 == 1)
                generate_cross();
        }

        // Generate crossings with corners.
        int i;
        for (i = 0; i + (len + 1) <= last_row; i += len + 1) {
            // Select crossing words.
            vector<pair<double, int> > sorted;
            for (int j = 0; j < (int)lengths[len].size(); j++)
                if (!used[lengths[len][j]]) {
                    double sum = 0.0;
                    for (int k = 0; k < len; k += 2)
                        sum += prob[words[lengths[len][j]][k] - 'A'];
                    sorted.push_back(pair<double, int>(sum, lengths[len][j]));
                }
            sort(sorted.begin(), sorted.end());
            if (sorted.size() < 8)
                return;
            vector<int> crossing_words(8);
            for (int j = 0; j < 4; j++) {
                crossing_words[2 * j] = sorted.back().second;
                cerr << "Wellness of " << words[sorted.back().second] << " is " <<
                     sorted.back().first << "\n";

                sorted.erase(sorted.end() - 1);
                int best = (int)sorted.size() - 1;
                double best_score = 0, seq;
                int p0, p1;
                // Special cases, don't want to have two matching letters
                // on the same positions.
                for (int k = (int)sorted.size() - 1; k >= 0; k--) {
                    double score = 0;
                    if (j == 0 || j == 3)
                        for (int p = 0; p < len; p += 2)
                            score += words[sorted[k].second][p] !=
                                     words[crossing_words[2 * j]][p];
                    int pos0, pos1;
                    if (j == 0)
                        pos0 = pos1 = len - 1;
                    if (j == 1)
                        pos0 = len - 1, pos1 = 0;
                    if (j == 2)
                        pos0 = len - 1, pos1 = 0;
                    if (j == 3)
                        pos0 = 0, pos1 = 0;
                    // The vowels should match.
                    //score += vowel[words[sorted[k].second][pos1] - 'A'] ==
                    //	vowel[words[crossing_words[2 * j]][pos0] - 'A'];
                    score += seq_prob[j]
                             [words[crossing_words[2 * j]][pos0] - 'A']
                             [words[sorted[k].second][pos1] - 'A'] / 2.5;
                    score += sorted[k].first;

                    if (score > best_score) {
                        seq = seq_prob[j]
                        [words[crossing_words[2 * j]][pos0] - 'A']
                        [words[sorted[k].second][pos1] - 'A'];

                        p0 = pos0, p1 = pos1;
                        best_score = score;
                        best = k;
                    }
                }
                cerr << "Wellness of " << words[sorted[best].second] << " is " <<
                     sorted[best].first << ", score is " << best_score << ", " <<
                     "seq_prob is " << seq << ", letters are " << p0 << " " << p1 << "\n";
                crossing_words[2 * j + 1] = sorted[best].second;
                sorted.erase(sorted.begin() + best);
            }

            int P = (N - 1) - i;
            put_word(i, i + len, 1, crossing_words[0]);
            put_word(i + len, i, 0, crossing_words[1]);
            put_word(i, P - len, 1, crossing_words[2]);
            put_word(i + len, P - (len - 1), 0, crossing_words[3]);
            put_word(P - len, i, 0, crossing_words[4]);
            put_word(P - (len - 1), i + len, 1, crossing_words[5]);
            put_word(P - len, P - (len - 1), 0, crossing_words[6]);
            put_word(P - (len - 1), P - len, 1, crossing_words[7]);

            // corners[i][j] = pair of matching words of length j
            // that can be placed into the i-th corner.
            vector<int> corners[4][16];

            //for (int j = 15; j >= 10; j--) {
            //	cerr << "Words of length " << j << ":\n";
            //	for (int k = 0; k < (int)lengths[j].size(); k++)
            //		cerr << words[lengths[j][k]] << "\n";
            //}

            // Try putting corners one by one.
            for (int j = i; j < i + (len + 1); j += 2) {
                cerr << "Putting " << j << "th row\n";

                // Corners of these lengths won't make troubles.
                bool found = 0;
                for (int q = min(15, (N - 2 * j - 1) / 2 - 1); q >= max(3, (len + 1) - (j - i)); q--)
                    if (q != len) {
                        int d = j - i;
                        int pos0 = len - d;
                        int pos1 = d + (q - 1) - len;
                        int pos2 = d;
                        int pos3 = (len - 1) - d;

                        possible_corner(j, q,
                                        words[crossing_words[0]][pos2] - 'A',
                                        words[crossing_words[1]][pos2] - 'A',
                                        pos0, pos0, 0, 0, corners[0][q]);
                        possible_corner(j, q,
                                        words[crossing_words[2]][pos2] - 'A',
                                        words[crossing_words[3]][pos3] - 'A',
                                        pos1, pos0, q - 1, 0, corners[1][q]);
                        possible_corner(j, q,
                                        words[crossing_words[4]][pos2] - 'A',
                                        words[crossing_words[5]][pos3] - 'A',
                                        pos1, pos0, q - 1, 0, corners[2][q]);
                        possible_corner(j, q,
                                        words[crossing_words[6]][pos3] - 'A',
                                        words[crossing_words[7]][pos3] - 'A',
                                        pos1, pos1, q - 1, q - 1, corners[3][q]);
                        if (!(
                            corners[0][q].empty() || corners[1][q].empty() ||
                            corners[2][q].empty() || corners[3][q].empty()
                        )) {
                            vector<int> octet(8);
                            octet[0] = corners[0][q][0];
                            octet[1] = corners[0][q][1];
                            octet[2] = corners[1][q][0];
                            octet[3] = corners[1][q][1];
                            octet[4] = corners[2][q][0];
                            octet[5] = corners[2][q][1];
                            octet[6] = corners[3][q][0];
                            octet[7] = corners[3][q][1];
                            put_word_octet(j, 0, octet);

                            found = 1;

                            break;
                        }
                        cerr << "Not all corners found\n";
                        for (int p = 0; p < 4; p++)
                            for (int k = 0; k < (int)corners[p][q].size(); k++) {
                                used[corners[p][q][k]] = 0;
                                cerr << "But found " << words[corners[p][q][k]] << "\n";
                            }
                    }
                if (!found) {
                    // Put anything.
                    for (int q = min(15, (N - 2 * (j + 1) - 1) / 2 - 1);
                         q >= max(3, len - (j - i)); q--
                        )
                        if (q != len) {
                            int d = j - i;
                            int pos0 = len - d - 1;
                            int pos1 = d + (q - 1) - len + 1;
                            int pos2 = d;
                            int pos3 = (len - 1) - d;

                            vector<int> octet(8);
                            octet[0] = possible_beginning(
                                words[crossing_words[0]][pos2] - 'A', pos0, q);
                            octet[1] = possible_beginning(
                                words[crossing_words[1]][pos2] - 'A', pos0, q);
                            octet[2] = possible_beginning(
                                words[crossing_words[2]][pos2] - 'A', pos1, q);
                            octet[3] = possible_beginning(
                                words[crossing_words[3]][pos3] - 'A', pos0, q);
                            octet[4] = possible_beginning(
                                words[crossing_words[4]][pos2] - 'A', pos1, q);
                            octet[5] = possible_beginning(
                                words[crossing_words[5]][pos3] - 'A', pos0, q);
                            octet[6] = possible_beginning(
                                words[crossing_words[6]][pos3] - 'A', pos1, q);
                            octet[7] = possible_beginning(
                                words[crossing_words[7]][pos3] - 'A', pos1, q);

                            found = 1;
                            for (int k = 0; k < 8; k++)
                                if (octet[k] == -1)
                                    found = 0;

                            if (found) {
                                cerr << "Putting anything\n";
                                put_word_octet(j, 1, octet);
                                break;
                            }
                            else {
                                for (int k = 0; k < 8; k++)
                                    if (octet[k] != -1)
                                        used[octet[k]] = 0;
                            }
                        }
                }
            }

        } // for (int i...

        for (; i <= last_row; i += 2)
            generate_composite_square(i);

        for (i = 0; i <= last_row; i += 2) {
            // Skip filled area.
            int r = (N - 1) / 2;
            while (r >= 0 && result[i][r] == '.')
                r--;
            r += 2;
            if (r <= i)
                r = i + 1;
            //cerr << "Putting middle " << i << " " << r << "\n";
            if (r < N / 2)
                generate_middle(i, r - i);
            //if (i % (len + 1) == (len - 1))
            //	place_over(i - (len - 1), len);
        }
    }
    bool replace(int w, int letter, int pos, vector<int> &use) {
        cerr << "In replace(" << w << ", " << (char)(letter + 'A') << ", " << pos << ")\n";
        if (w < 0 || crossed[w])
            return 0;

        int len = words[w].size();

        cerr << "Searching for a replacement for " << words[w] << " with "
             << pos << "th letter " << (char)(letter + 'A') << "\n";
        cerr << "Looking through " << by_letter[letter][pos][len].size() << " words\n";

        if (words[w][pos] - 'A' == letter) {
            cerr << "It match\n";
            use.push_back(w);
            return 1;
        }

        for (int i = 0; i < (int)by_letter[letter][pos][len].size(); i++) {
            int r = by_letter[letter][pos][len][i];
            if (!used[r] || !crossed[r]) {
                cerr << "Found " << words[r] << "\n";

                pair<int, int> coord = coordinates[w];
                int dir = direction[w];
                unput_word(w);
                coordinates[w] = pair<int, int>(-1, -1);

                if (used[r]) {
                    unput_word(r);
                    put_word(coordinates[r].first, coordinates[r].second,
                             direction[r], w, 1);
                    coordinates[r] = pair<int, int>(-1, -1);
                }

                //cerr << "Unput\n";
                put_word(coord.first, coord.second, dir, r, 1);
                crossed[r] = 1;
                //cerr << "Put\n";
                use.push_back(r);
                return 1;
            }
        }
        return 0;
    }

    void place_over(int i, int len) {
        int max_row = (N - 1) / 2 - 1;

        // Find possible places, such that all crossed words
        // were uncrossed before this iteration.

        for (int j = i; j <= max_row; j++) {
            bool valid = i + (len - 1) < N / 2;
            // i-th row always starts the group of size (len + 1) / 2 rows.
            for (int k = 0; k < len && valid; k++) {
                if (result[i + k][j] == '.' && (
                    (j > 0 && result[i + k][j - 1] != '.') ||
                    (j < N - 1 && result[i + k][j + 1] != '.')
                )) valid = 0;
                if (which_word[i + k][j] == -1 || (
                    which_word[i + k][j] >= 0 && crossed[which_word[i + k][j]]
                )) valid = 0;
            }

            //for (int p = 0; p < N; p++) {
            //	for (int q = 0; q < N; q++) {
            //		if (which_word[p][q] == -2)
            //			cerr << '.';
            //		else if (which_word[p][q] == -1)
            //			cerr << '#';
            //		else
            //			cerr << '+';
            //	}
            //	cerr << "\n";
            //}

            if (valid && result[i][j] != '.') {
                // Try to place 8 crossings.
                cerr << "Attempt to place additional 8 crossings in row " <<
                     i << " column " << j << "\n";

                // Select crossing words.
                vector<pair<double, int> > sorted;
                for (int p = 0; p < (int)lengths[len].size(); p++)
                    if (!used[lengths[len][p]]) {
                        double sum = 0.0;
                        for (int k = 0; k < len; k += 2)
                            sum += prob[words[lengths[len][p]][k] - 'A'];
                        sorted.push_back(pair<double, int>(sum, lengths[len][p]));
                    }
                sort(sorted.begin(), sorted.end());
                if (sorted.size() < 8)
                    continue;
                vector<int> crossing_words(8);
                for (int p = 0, q = (int)sorted.size() - 1; p < 8; p++, q--) {
                    crossing_words[p] = sorted[q].second;
                    //cerr << "Wellness of " << words[sorted[q].second] << " is " <<
                    //	sorted[q].first << "\n";
                }

                vector<int> use;
                for (int k = 0; k < len && valid; k += 2) {
                    // If there is a gap, skip this row.
                    if (which_word[i + k][j] == -2)
                        continue;

                    cerr << "0\n";
                    valid &= replace(
                        which_word[i + k][j],
                        words[crossing_words[0]][k] - 'A',
                        j - coordinates[which_word[i + k][j]].second,
                        use);

                    cerr << "1\n";
                    valid &= replace(
                        which_word[j][i + k],
                        words[crossing_words[1]][k] - 'A',
                        j - coordinates[which_word[j][i + k]].first,
                        use);

                    cerr << "2\n";
                    //cerr << "param = " << i + k << " " << (N - 1) - j << "\n";
                    //cerr << "word = " << which_word[i + k][(N - 1) - j] << "\n";
                    valid &= replace(
                        which_word[i + k][(N - 1) - j],
                        words[crossing_words[2]][k] - 'A',
                        (N - 1) - j - coordinates[which_word[i + k][(N - 1) - j]].second,
                        use);

                    cerr << "3\n";
                    //cerr << "param = " << (N - 1) - (i + k) << "\n";
                    valid &= replace(
                        which_word[j][(N - 1) - (i + k)],
                        words[crossing_words[3]][(len - 1) - k] - 'A',
                        j - coordinates[which_word[j][(N - 1) - (i + k)]].first,
                        use);

                    cerr << "4\n";
                    //cerr << "param = " << (N - 1) - j << " " << i + k << "\n";
                    //cerr << "word = " << which_word[(N - 1) - j][i + k] << "\n";
                    valid &= replace(
                        which_word[(N - 1) - j][i + k],
                        words[crossing_words[4]][k] - 'A',
                        (N - 1) - j - coordinates[which_word[(N - 1) - j][i + k]].first,
                        use);

                    cerr << "5\n";
                    //cerr << "param = " << (N - 1) - j << " " << i + k << "\n";
                    valid &= replace(
                        which_word[(N - 1) - (i + k)][j],
                        words[crossing_words[5]][(len - 1) - k] - 'A',
                        j - coordinates[which_word[(N - 1) - (i + k)][j]].second,
                        use);

                    cerr << "6\n";
                    valid &= replace(
                        which_word[(N - 1) - j][(N - 1) - (i + k)],
                        words[crossing_words[6]][(len - 1) - k]  - 'A',
                        (N - 1) - j -
                        coordinates[which_word[(N - 1) - j][(N - 1) - (i + k)]].first,
                        use);

                    cerr << "7\n";
                    valid &= replace(
                        which_word[(N - 1) - (i + k)][(N - 1) - j],
                        words[crossing_words[7]][(len - 1) - k] - 'A',
                        (N - 1) - j -
                        coordinates[which_word[(N - 1) - (i + k)][(N - 1) - j]].second,
                        use);
                }
                if (valid) {
                    cerr << "Found crossing middle\n";
                    put_word(i, j, 1, crossing_words[0]);
                    put_word(j, i, 0, crossing_words[1]);
                    put_word(i, (N - 1) - j, 1, crossing_words[2]);
                    put_word(j, (N - 1) - i - (len - 1), 0, crossing_words[3]);
                    put_word((N - 1) - j, i, 0, crossing_words[4]);
                    put_word((N - 1) - i - (len - 1), j, 1, crossing_words[5]);
                    put_word((N - 1) - j, (N - 1) - i - (len - 1), 0, crossing_words[6]);
                    cerr << (N - 1) - i - (len - 1) << " " << (N - 1) - j << "\n";
                    cerr << crossing_words[7] << "\n";
                    put_word((N - 1) - i - (len - 1), (N - 1) - j, 1, crossing_words[7]);
                }
                else
                    for (int k = 0; k < (int)use.size(); k++)
                        crossed[use[k]] = 0;
            }
        }
    }
    void show_statistics() {
        int left = W;
        for (int i = 0; i < W; i++)
            left -= used[i];
        cerr << left << " words left unused\n";
        cerr << total_used << " words of " << W << " are used\n";

        for (int i = 3; i <= L; i++) {
            int count = 0;
            for (int j = 0; j < (int)lengths[i].size(); j++)
                count += !used[lengths[i][j]];
            cerr << count << " words of length " << i << " are unused\n";
        }

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++)
                cerr << result[i][j];
            cerr << "\n";
        }
    }

    double score() {
        double r = 0.0;

        int filled = 0;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                if (result[i][j] != '.')
                    filled++;
        r += weights[0] * (1.0 * filled) / (N * N);

        cerr << "Board filling: " << (1.0 * filled) / (N * N) << "\n";

        int frows = 0, fcols = 0;
        for (int i = 0; i < N; i++) {
            bool frow = 0, fcol = 0;
            for (int j = 0; j < N; j++) {
                frow |= result[i][j] != '.';
                fcol |= result[j][i] != '.';
            }
            frows += frow;
            fcols += fcol;
        }
        r += weights[1] * (1.0 * frows * fcols) / (N * N);

        cerr << "Row/col filling: " << (1.0 * frows * fcols) / (N * N) << "\n";

        double ssum = 0.0;
        int eight_count = 0;
        for (int i = 0; i <= N - i - 1; i++)
            for (int j = 0; j <= i; j++) {
                eight_count++;
                int c = (result[i][j] == '.') +
                        (result[j][i] == '.') +
                        (result[N - i - 1][j] == '.') +
                        (result[j][N - i - 1] == '.') +
                        (result[i][N - j - 1] == '.') +
                        (result[N - j - 1][i] == '.') +
                        (result[N - i - 1][N - j - 1] == '.') +
                        (result[N - j - 1][N - i - 1] == '.');
                if (c == 0 || c == 8)
                    ssum += 1;
                if (c == 1 || c == 7)
                    ssum += 0.5;
                if (c == 2 || c == 6)
                    ssum += 0.1;
            }
        r += weights[2] * ssum / eight_count;

        cerr << "Symmetry: " << ssum / eight_count << "\n";

        double crossings = 0;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                if (result[i][j] != '.'
                    && (i > 0 && result[i - 1][j] != '.' || i < N - 1
                                                            && result[i + 1][j] != '.')
                    && (j > 0 && result[i][j - 1] != '.' || j < N - 1
                                                            && result[i][j + 1] != '.'))
                    crossings++;
        if (filled > 0)
            crossings /= filled;
        r += weights[3] * crossings;

        cerr << "Crossing: " << crossings << "\n";

        return r / (weights[0] + weights[1] + weights[2] + weights[3]);
    }

    bool valid() {
        // TODO
    }
    void cleanup() {
        this->result = vector<string>(N, string(N, '.'));
        this->used = vector<int>(W);

        which_word = vector<vector<int> >(N, vector<int>(N, -2));
        coordinates = vector<pair<int, int> >(W, pair<int, int>(-1, -1));
        direction = vector<int>(W, 0);
        crossed = vector<int>(W, 0);

        total_used = 0;

        for (int i = 0; i < 26; i++)
            prob[i] = 0;
        int total = 0;
        for (int i = 0; i < (int)words.size(); i++)
            for (int j = 0; j < (int)words[i].size(); j++) {
                total++;
                prob[words[i][j] - 'A']++;
            }
        for (int i = 0; i < 26; i++)
            prob[i] /= total;

        for (int i = 0; i < 26; i++)
            vowel[i] = 0;
        vowel['A' - 'A'] = vowel['E' - 'A'] = vowel['I' - 'A'] =
        vowel['O' - 'A'] = vowel['U' - 'A'] = 1;

        for (int k = 0; k < 4; k++)
            for (int i = 0; i < 26; i++)
                for (int j = 0; j < 26; j++)
                    seq_prob[k][i][j] = 0;
        for (int i = 0; i < 26; i++)
            for (int j = 0; j < 26; j++)
                for (int len = 3; len <= L; len++)
                    for (int p = 0; p < (int)by_letter[i][1][len].size(); p++)
                        for (int q = 0; q < (int)by_letter[j][1][len].size(); q++)
                            seq_prob[0][i][j] +=
                                by_letter[i][1][len][p] !=
                                by_letter[j][1][len][q] &&
                                words[by_letter[i][1][len][p]][0] ==
                                words[by_letter[j][1][len][q]][0];
        for (int i = 0; i < 26; i++)
            for (int j = 0; j < 26; j++)
                for (int len = 3; len <= L; len++)
                    for (int p = 0; p < (int)by_letter[i][(len - 1) - 1][len].size(); p++)
                        for (int q = 0; q < (int)by_letter[j][1][len].size(); q++)
                            seq_prob[1][i][j] +=
                                by_letter[i][(len - 1) - 1][len][p] !=
                                by_letter[j][1][len][q] &&
                                words[by_letter[i][(len - 1) - 1][len][p]][len - 1] ==
                                words[by_letter[j][1][len][q]][0];
        for (int i = 0; i < 26; i++)
            for (int j = 0; j < 26; j++)
                for (int len = 3; len <= L; len++)
                    for (int p = 0; p < (int)by_letter[i][(len - 1) - 1][len].size(); p++)
                        for (int q = 0; q < (int)by_letter[j][1][len].size(); q++)
                            seq_prob[2][i][j] +=
                                by_letter[i][(len - 1) - 1][len][p] !=
                                by_letter[j][1][len][q] &&
                                words[by_letter[i][(len - 1) - 1][len][p]][len - 1] ==
                                words[by_letter[j][1][len][q]][0];
        for (int i = 0; i < 26; i++)
            for (int j = 0; j < 26; j++)
                for (int len = 3; len <= L; len++)
                    for (int p = 0; p < (int)by_letter[i][(len - 1) - 1][len].size(); p++)
                        for (int q = 0; q < (int)by_letter[j][(len - 1) - 1][len].size(); q++)
                            seq_prob[3][i][j] +=
                                by_letter[i][(len - 1) - 1][len][p] !=
                                by_letter[j][(len - 1) - 1][len][q] &&
                                words[by_letter[i][(len - 1) - 1][len][p]][len - 1] ==
                                words[by_letter[j][(len - 1) - 1][len][q]][0];
        for (int k = 0; k < 4; k++)
            for (int i = 0; i < 26; i++)
                for (int j = 0; j < 26; j++) {
                    seq_prob[k][i][j] /= 10000;
                    if (seq_prob[k][i][j] > 0.03)
                        cerr << k << "th corner probability for " << (char)(i + 'A') <<
                             (char)(j + 'A') << " = " << seq_prob[k][i][j] << "\n";
                }
    }

public:
    vector<string> generate(int N, vector<string> &words, vector<int> &weights) {
        this->N = N;
        this->W = words.size();
        this->weights = weights;
        this->words = words;

        L = 0;
        for (int i = 0; i < (int)words.size(); i++) {
            this->lengths[words[i].size()].push_back(i);

            L = max(L, (int)words[i].size());
            for (int j = 0; j < (int)words[i].size(); j++)
                this->by_letter[words[i][j] - 'A'][j][words[i].size()].push_back(i);
        }

        for (int i = 3; i <= L; i++)
            cerr << "There are " << this->lengths[i].size() << " words of length " <<
                 i << "\n";

        int start;

        cleanup();
        cerr << "Using algorithm generate_squares\n";
        start = clock();
        generate_squares();
        int time0 = clock() - start;
        double score_squares = score();
        vector<string> result_squares(result);
        show_statistics();

        cleanup();
        cerr << "Using algorithm generate_corner_squares\n";
        start = clock();
        generate_corner_squares();
        vector<string> result_corner_squares(result);
        int time1 = clock() - start;
        double score_corner_squares = score();
        show_statistics();

        cerr << "Using algorithm generate_crossings\n";
        start = clock();
        double score_crossings = -1;
        vector<string> result_crossings(result);
        for (int len = 9; len >= 5; len -= 2) {
            cleanup();
            generate_crossings(len);
            double now = score();
            if (now > score_crossings) {
                score_crossings = now;
                result_crossings = result;
            }
        }
        result = result_crossings;
        int time2 = clock() - start;

        show_statistics();

        if (score_corner_squares > score_squares && score_corner_squares > score_crossings)
            result = result_corner_squares;
        if (score_squares > score_corner_squares && score_squares > score_crossings)
            result = result_squares;

        cerr << "score for algorithm generate_squares = " << score_squares << "\n";
        cerr << "time: " << 1.0 * time0 / CLOCKS_PER_SEC << "\n";
        cerr << "score for algorithm generate_corner_squares = " << score_corner_squares << "\n";
        cerr << "time: " << 1.0 * time1 / CLOCKS_PER_SEC << "\n";
        cerr << "Score for algorithm generate_crossings = " << score_crossings << "\n";
        cerr << "Time: " << 1.0 * time2 / CLOCKS_PER_SEC << "\n";


        return result;
    }
};


class BeautifulCrossword
{
public:
    vector<string> generateCrossword(int N, vector <string> words, vector <int> weights);
};

vector<string> BeautifulCrossword::generateCrossword(int N, vector <string> words, vector <int> weights)
{
    return crossword().generate(N, words, weights);
}