/**
 * abcd_adr_waveform_exporter.cpp
 *
 * Standalone utility to extract digitized waveforms from ABCD DAQ
 * binary (.adr) files and export them to CSV format.
 *
 * The code supports:
 *  - exporting waveforms from a single selected channel
 *  - exporting waveforms from all channels
 *  - optional waveform limits per channel
 *
 * This example is adapted from PhD analysis work and provided without
 * experimental data. It demonstrates binary parsing, efficient I/O,
 * and DAQ-level data handling.
 *
 * Author: Ali F. Alwars
 *
 * Compile:
 *   g++ -std=c++17 -O2 abcd_adr_waveform_exporter.cpp -o export_wf
 *
 * Usage:
 *   ./export_wf
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <cstdint>

// ------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------

struct WaveformPacket {
    uint64_t timestamp;
    uint8_t  channel;
    uint32_t sample_count;
    uint8_t  gates_count;
    std::vector<uint16_t> samples;
};

static bool read_waveform_packet(const std::vector<char>& buffer,
                                 size_t& pos,
                                 WaveformPacket& packet)
{
    if (pos + 14 > buffer.size())
        return false;

    std::memcpy(&packet.timestamp, &buffer[pos], 8); pos += 8;
    std::memcpy(&packet.channel,   &buffer[pos], 1); pos += 1;
    std::memcpy(&packet.sample_count, &buffer[pos], 4); pos += 4;
    std::memcpy(&packet.gates_count,  &buffer[pos], 1); pos += 1;

    if (pos + packet.sample_count * 2 > buffer.size())
        return false;

    packet.samples.resize(packet.sample_count);
    for (uint32_t i = 0; i < packet.sample_count; ++i) {
        std::memcpy(&packet.samples[i], &buffer[pos], 2);
        pos += 2;
    }

    // Remove trailing samples (hardware-specific padding)
    if (packet.samples.size() >= 4)
        packet.samples.resize(packet.samples.size() - 4);

    return true;
}

static void write_samples_csv(std::ofstream& out,
                              const std::vector<uint16_t>& samples)
{
    for (size_t i = 0; i < samples.size(); ++i) {
        if (i > 0) out << ",";
        out << samples[i];
    }
    out << "\n";
}

// ------------------------------------------------------------
// Export single channel
// ------------------------------------------------------------

void export_single_channel(const std::string& input_file,
                           int channel_id,
                           int max_waveforms)
{
    clock_t start = clock();

    std::ifstream in(input_file, std::ios::binary);
    if (!in) {
        std::cerr << "Error: cannot open " << input_file << "\n";
        return;
    }

    std::string base = input_file.substr(0, input_file.find(".adr"));
    std::string csv_name = base + "_wf_ch" + std::to_string(channel_id) + ".csv";
    std::ofstream out(csv_name);

    if (!out) {
        std::cerr << "Error: cannot create " << csv_name << "\n";
        return;
    }

    std::cout << "Exporting channel " << channel_id
              << " → " << csv_name << "\n";

    char c;
    std::string topic;
    int exported = 0;

    while (in.get(c)) {
        if (c != ' ') {
            topic += c;
            continue;
        }

        size_t size_pos = topic.rfind("_s");
        if (size_pos == std::string::npos) {
            topic.clear();
            continue;
        }

        int msg_size = std::stoi(topic.substr(size_pos + 2));
        std::vector<char> buffer(msg_size);

        if (!in.read(buffer.data(), msg_size)) {
            topic.clear();
            continue;
        }

        if (topic.find("data_abcd_waveforms") == 0) {
            size_t pos = 0;
            while (pos < buffer.size()) {
                WaveformPacket pkt;
                if (!read_waveform_packet(buffer, pos, pkt))
                    break;

                if (pkt.channel == channel_id) {
                    write_samples_csv(out, pkt.samples);
                    exported++;

                    if (exported % 10000 == 0)
                        std::cout << "  exported " << exported << " waveforms\n";

                    if (max_waveforms > 0 && exported >= max_waveforms)
                        goto done;
                }
            }
        }

        topic.clear();
    }

done:
    std::cout << "Finished. Exported " << exported << " waveforms\n";
    std::cout << "Elapsed time: "
              << double(clock() - start) / CLOCKS_PER_SEC
              << " s\n";
}

// ------------------------------------------------------------
// Export all channels
// ------------------------------------------------------------

void export_all_channels(const std::string& input_file,
                         int max_per_channel,
                         int exclude_channel)
{
    clock_t start = clock();

    std::ifstream in(input_file, std::ios::binary);
    if (!in) {
        std::cerr << "Error: cannot open " << input_file << "\n";
        return;
    }

    std::string base = input_file.substr(0, input_file.find(".adr"));
    std::map<int, std::ofstream> outputs;
    std::map<int, int> counts;

    char c;
    std::string topic;

    while (in.get(c)) {
        if (c != ' ') {
            topic += c;
            continue;
        }

        size_t size_pos = topic.rfind("_s");
        if (size_pos == std::string::npos) {
            topic.clear();
            continue;
        }

        int msg_size = std::stoi(topic.substr(size_pos + 2));
        std::vector<char> buffer(msg_size);

        if (!in.read(buffer.data(), msg_size)) {
            topic.clear();
            continue;
        }

        if (topic.find("data_abcd_waveforms") == 0) {
            size_t pos = 0;
            while (pos < buffer.size()) {
                WaveformPacket pkt;
                if (!read_waveform_packet(buffer, pos, pkt))
                    break;

                int ch = static_cast<int>(pkt.channel);
                if (ch == exclude_channel)
                    continue;

                if (!outputs.count(ch)) {
                    std::string name = base + "_wf_ch" + std::to_string(ch) + ".csv";
                    outputs[ch].open(name);
                    counts[ch] = 0;
                    std::cout << "Created " << name << "\n";
                }

                if (max_per_channel <= 0 || counts[ch] < max_per_channel) {
                    write_samples_csv(outputs[ch], pkt.samples);
                    counts[ch]++;
                }
            }
        }

        topic.clear();
    }

    std::cout << "Finished exporting waveforms\n";
    for (const auto& kv : counts)
        std::cout << "  Channel " << kv.first
                  << ": " << kv.second << " waveforms\n";

    std::cout << "Elapsed time: "
              << double(clock() - start) / CLOCKS_PER_SEC
              << " s\n";
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------

int main()
{
    std::cout << "=== ABCD ADR Waveform Exporter ===\n";

    std::string filename;
    std::cout << "Input ADR file: ";
    std::getline(std::cin, filename);

    int channel;
    std::cout << "Channel (-1 = all): ";
    std::cin >> channel;

    int max_wf;
    std::cout << "Max waveforms (0 = all): ";
    std::cin >> max_wf;

    if (channel == -1) {
        int exclude;
        std::cout << "Exclude channel (-1 = none): ";
        std::cin >> exclude;
        export_all_channels(filename,
                            max_wf == 0 ? -1 : max_wf,
                            exclude);
    } else {
        export_single_channel(filename,
                              channel,
                              max_wf == 0 ? -1 : max_wf);
    }

    return 0;
}
