#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <RtAudio.h>

using namespace cv;
using namespace std;
using namespace chrono;
using namespace boost;
namespace fs = filesystem;
namespace po = program_options;
namespace pt = posix_time;

//****************************************************
// DECLARATIONS
//****************************************************

struct AudioStruct
{
	unsigned int channels;
	unsigned int samplerate;
	unsigned int buffersize;
	ofstream file;
};

struct CaptureParameters
{
	vector<double> fps;
	vector<double> height;
	vector<double> width;
	vector<string> fourcc;
};

void audioDeviceParameters(vector<AudioStruct>& aud_strct,
                           vector<RtAudio::StreamParameters>& stream_pram,
                           vector<int>& dev_vec,
                           unsigned int& ar, unsigned int& ac,
                           unsigned int& ab,
                           vector<string>& file_str);

void removeDirectory(string& folder);

void audioDataSize(ofstream& file);

void audioHeader(ofstream& file, int& bit_depth,
                 unsigned int& ac, unsigned int& ar);

int audioWriter(void* outputBuffer, void* inputBuffer,
                unsigned int nBufferFrames, double streamTime,
                RtAudioStreamStatus status, void* data);

void colorizeMat(Mat& img);

int findNearestTimeStamp(double tick, vector<int> ticks);

Mat imDisplay(string title, vector<Mat>& imgs,
              Size& imSize, Size& gridSize, double& scaler, int& rec);

vector<VideoCapture> initCapDevices(const vector<int> usb_ids,
                                    const vector<string> url_ids,
                                    vector<Mat>& imgs,
                                    CaptureParameters& prams);

vector<VideoWriter> initWriteDevices(vector<VideoCapture>& capDevices,
                                     vector<string>& paths,
                                     double& fps,
                                     string& four_cc,
                                     string& ext);

void fillCapSetOpts(int& ncams, vector<double>& fps,
                    vector<double>& height, vector<double>& width,
                    vector<string>& fourcc);

string filePrefix();

vector<string> makeDirectory(string& file_prefix, string& root_path);

void mergeMatVectors(vector<vector<Mat>>& v0,
                     vector<vector<Mat>>& v1, vector<vector<Mat>>& v2);

void mergeTimestamps(vector<vector<int>>& v0,
                     vector<vector<int>>& v1, vector<vector<int>>& v2);

int msTimestamp(time_point<high_resolution_clock>& tick);

void setCapParameters(int idx, vector<VideoCapture>& device,
                      CaptureParameters& prams);

void printCapParameters(vector<VideoCapture>& device);

void printTime(string idx, time_point<high_resolution_clock> ts);

void recSwitch(int recPosition, void*);

void removeDirectory(string& folder);

void videoDisplaySetup(Size& grid, Size& imdim, vector<Mat>& imgs,
                       double& scaler, int c_row, int c_col);

void waitMilliseconds(int t);

void writeCSVHeaders(ofstream& file, vector<string> name);

void writeTimeStampData(ofstream& file, vector<double>& timestamps);


//****************************************************
// DEFINITIONS
//****************************************************

void audioDeviceParameters(vector<AudioStruct>& aud_strct,
                           vector<RtAudio::StreamParameters>& stream_pram,
                           vector<int>& dev_vec,
                           unsigned int& ar, unsigned int& ac,
                           unsigned int& ab,
                           vector<string>& file_str)
{
	for (auto i = 0; i < dev_vec.size(); i++)
	{
		aud_strct[i].samplerate = ar;
		aud_strct[i].channels = ac;
		aud_strct[i].buffersize = ab;
		aud_strct[i].file.open(
			            file_str[0] + "/audio_" + to_string(i) + "_16bit_" +
			            to_string(ar) + "Hz_" + to_string(ac) + "ch_" +
			            file_str[1] + ".wav", ios::binary
		            );

		stream_pram[i].deviceId = dev_vec[i];
		stream_pram[i].nChannels = ac;
		stream_pram[i].firstChannel = 0;
	}
}

void audioDataSize(ofstream& file)
{
	file.seekp(0, ios::beg);
	streampos fbeg = file.tellp();
	file.seekp(0, ios::end);
	auto fsize = static_cast<unsigned int>(file.tellp() - fbeg);
	unsigned int overallSize = fsize - 8;
	unsigned int dataPortion = fsize - 44;
	file.seekp(4);
	file.write(reinterpret_cast<char *>(&overallSize), 4);
	file.seekp(40);
	file.write(reinterpret_cast<char *>(&dataPortion), 4);
}

void audioHeader(ofstream& file, int& bit_depth, unsigned int& ac, unsigned int& ar)
{
	unsigned int data_header_length = 16;
	unsigned int initSize = 0;
	unsigned short pcmType = 1;
	unsigned int bytesPerSec = (ar * bit_depth * ac) / 8;
	unsigned short bytesPerFrame = (bit_depth * ac) / 8;

	file.write("RIFF", 4);
	file.write(reinterpret_cast<char *>(&initSize), 4);
	file.write("WAVE", 4);
	file.write("fmt ", 4);
	file.write(reinterpret_cast<char *>(&data_header_length), 4);
	file.write(reinterpret_cast<char *>(&pcmType), 2);
	file.write(reinterpret_cast<char *>(&ac), 2);
	file.write(reinterpret_cast<char *>(&ar), 4);
	file.write(reinterpret_cast<char *>(&bytesPerSec), 4);
	file.write(reinterpret_cast<char *>(&bytesPerFrame), 2);
	file.write(reinterpret_cast<char *>(&bit_depth), 2);
	file.write("data", 4);
	file.write(reinterpret_cast<char *>(&initSize), 4);
}

int audioWriter(void* outputBuffer, void* inputBuffer,
                unsigned int nBufferFrames, double streamTime,
                RtAudioStreamStatus status, void* data)
{
	if (status) cout << "Stream overflow detected!" << endl;
	auto inputData = static_cast<AudioStruct *>(data);
	inputData->file.write(static_cast<char *>(inputBuffer),
	                      sizeof(signed short) * nBufferFrames * inputData->channels);
	return 0;
}

void colorizeMat(Mat& img)
{
	for (auto m = 0; m < img.rows; m++)
	{
		for (auto n = 0; n < img.cols; n++)
		{
			img.at<Vec3b>(m, n)[0] = 0;
			img.at<Vec3b>(m, n)[1] = 0;
			img.at<Vec3b>(m, n)[2] = 255;
		}
	}
}

int findNearestTimeStamp(double tick, vector<int> ticks)
{
	vector<double> tickDiff(ticks.size());
	for (auto i = 0; i < ticks.size(); i++)
	{
		tickDiff[i] = abs(static_cast<double>(ticks[i]) - tick);
	}
	vector<double>::iterator mins = min_element(tickDiff.begin(), tickDiff.end());
	auto min_index = static_cast<int>(distance(tickDiff.begin(), mins));
	return min_index;
}

Mat imDisplay(string title, vector<Mat>& imgs,
              Size& imSize, Size& gridSize, double& scaler, int& rec)
{
	Mat out_img = Mat::zeros(imSize.height, imSize.width, CV_8UC3);
	auto n_imgs = static_cast<int>(imgs.size());
	int img_idx = 0;
	int out_h_start = 0;
	int out_w_start;
	int out_h_end = 0;
	int out_w_end = 0;

	for (auto m = 0; m < gridSize.height; m++)
	{
		out_w_start = 0;
		for (auto n = 0; n < gridSize.width; n++)
		{
			if (img_idx < n_imgs)
			{
				Mat in_img;

				resize(imgs[img_idx], in_img,
				       Size(), scaler, scaler, INTER_NEAREST);

				Size in_img_sz = in_img.size();

				out_h_end = out_h_start + (in_img_sz.height - 1);
				out_w_end = out_w_start + (in_img_sz.width - 1);

				Range in_roi_h = Range(0, in_img_sz.height - 1);
				Range in_roi_w = Range(0, in_img_sz.width - 1);

				Range out_roi_w = Range(out_w_start, out_w_end);
				Range out_roi_h = Range(out_h_start, out_h_end);

				//cout << "\n--IN RECT:" << img_idx << endl;
				//cout << in_roi_w.start << endl;
				//cout << in_roi_h.start << endl;
				//cout << in_roi_w.end << endl;
				//cout << in_roi_h.end << endl;
				//cout << "\n--OUT RECT:" << img_idx << endl;
				//cout << out_roi_w.start << endl;
				//cout << out_roi_h.start << endl;
				//cout << out_roi_w.end << endl;
				//cout << out_roi_h.end << endl;

				in_img(in_roi_h, in_roi_w).copyTo(out_img(out_roi_h, out_roi_w));
			}
			img_idx++;
			out_w_start = out_w_end + 2;
		}
		out_h_start = out_h_end + 2;
	}

	if (rec == 1)
	{
		circle(out_img, Point(15, 15), 14, Scalar(0, 0, 255), -1, 8);
		putText(out_img, "REC", Point(30, 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255));
	}
	namedWindow(title);
	imshow(title, out_img);
	return out_img;
}

void setCapParameters(int idx, vector<VideoCapture>& device, CaptureParameters& prams)
{
	if (prams.fps[idx] != 0)
	{
		device[idx].set(CAP_PROP_FPS, prams.fps[idx]);
	}
	if (prams.height[idx] != 0)
	{
		device[idx].set(CAP_PROP_FRAME_HEIGHT, prams.height[idx]);
	}
	if (prams.width[idx] != 0)
	{
		device[idx].set(CAP_PROP_FRAME_WIDTH, prams.width[idx]);
	}
	if (prams.fourcc[idx].compare("") != 0 && prams.fourcc[idx].size() == 4)
	{
		auto codec = VideoWriter::fourcc(prams.fourcc[idx][0], prams.fourcc[idx][1], prams.fourcc[idx][2], prams.fourcc[idx][3]);
		device[idx].set(CAP_PROP_FOURCC, codec);
	}
}

void printCapParameters(vector<VideoCapture>& device)
{
	for (auto i = 0; i < device.size(); ++i)
	{
		cout << "\ndevice " << i + 1
			<< " capture parameter: CAP_PROP_FPS "
			<< device[i].get(CAP_PROP_FPS) << endl;
		cout << "device " << i + 1
			<< " capture parameter: CAP_PROP_FRAME_HEIGHT "
			<< device[i].get(CAP_PROP_FRAME_HEIGHT) << endl;
		cout << "device " << i + 1
			<< " capture parameter: CAP_PROP_FRAME_WIDTH "
			<< device[i].get(CAP_PROP_FRAME_WIDTH) << endl;
		cout << "device " << i + 1
			<< " capture parameter: CAP_PROP_FOURCC "
			<< device[i].get(CAP_PROP_FOURCC) << endl;
	}
}

vector<VideoCapture> initCapDevices(const vector<int> usb_ids, const vector<string> url_ids, vector<Mat>& imgs, CaptureParameters& prams)
{
	size_t n_usb = usb_ids.size();
	size_t n_url = url_ids.size();
	vector<VideoCapture> devices(n_usb + n_url);
	int device_counter = 0;

	if (n_usb != 0)
	{
		for (int i = 0; i < n_usb; i++)
		{
			cout << "\nTrying to load USB device: " << usb_ids[i] << endl;

			Mat tmpImg;
			int capCounter = 0;
			bool capFail = true;

			while (capFail)
			{
				++capCounter;

				if (capCounter > 9)
				{
					cerr << "failed to open device " << usb_ids[i] << " too many times!" << endl;
					throw;
				}

				devices[device_counter].open(usb_ids[i]);


				if (!devices[device_counter].isOpened())
				{
					cerr << "Cam not found with ID: " << usb_ids[i] << endl;
					continue;
				}

				if (!devices[device_counter].grab())
				{
					cerr << "Frame was not grabbed successfully for USB cam: " << usb_ids[i] << endl;
					devices[device_counter].release();
					continue;
				}

				if (!devices[device_counter].retrieve(tmpImg))
				{
					cerr << "Frame was not decoded successfully for USB cam: " << usb_ids[i] << endl;
					devices[device_counter].release();
					continue;
				}

				setCapParameters(device_counter, devices, prams);
				devices[device_counter] >> tmpImg;
				cout << "Loading of usb device: " << usb_ids[i] << " ...successful!" << endl;
				capFail = false;
			}

			imgs[device_counter] = tmpImg.clone();
			device_counter++;
		}
	}

	if (n_url != 0)
	{
		for (int i = 0; i < n_url; i++)
		{
			cout << "\nTrying IP url:\n" << url_ids[i] << endl;

			Mat tmpImg;
			int capCounter = 0;
			bool capFail = true;

			while (capFail)
			{
				++capCounter;

				if (capCounter > 10)
				{
					cerr << "failed to open url too many times:\n" << url_ids[i] << endl;
					throw;
				}

				devices[device_counter].open(url_ids[i]);

				if (!devices[device_counter].isOpened())
				{
					cerr << "Cam not found with url:\n" << url_ids[i] << endl;
					continue;
				}

				if (!devices[device_counter].grab())
				{
					cerr << "Frame was not grabbed successfully for IP cam: " << url_ids[i] << endl;
					devices[device_counter].release();
					continue;
				}

				if (!devices[device_counter].retrieve(tmpImg))
				{
					cerr << "Frame was not decoded successfully for IP cam: " << url_ids[i] << endl;
					devices[device_counter].release();
					continue;
				}

				devices[device_counter].open(url_ids[i]);

				cout << "Loading of IP url successful!:\n" << url_ids[i] << endl;
				capFail = false;
			}

			imgs[device_counter] = tmpImg.clone();
			device_counter++;
		}
	}

	return devices;
}

vector<VideoWriter> initWriteDevices(vector<VideoCapture>& capDevices,
                                     vector<string>& paths,
                                     double& fps,
                                     string& four_cc,
                                     string& ext)
{
	vector<VideoWriter> vidWriteVec(capDevices.size());
	auto codec = VideoWriter::fourcc(four_cc[0], four_cc[1], four_cc[2], four_cc[3]);
	string fileExt("." + ext);
	string fileStr;

	for (auto i = 0; i < capDevices.size(); i++)
	{
		Mat tempImg;
		capDevices[i] >> tempImg;
		fileStr = paths[0] + "/camera_" + to_string(i) + "_" + paths[1] + fileExt;
		vidWriteVec[i].open(fileStr, codec, fps, tempImg.size(), true);
	}
	return vidWriteVec;
}

void fillCapSetOpts(int ncams, vector<double>& fps, vector<double>& height, vector<double>& width, vector<string>& fourcc)
{
	for (auto i = 0; i < ncams - fps.size(); ++i)
	{
		fps.push_back(0);
	}

	for (auto i = 0; i < ncams - height.size(); ++i)
	{
		height.push_back(0);
	}

	for (auto i = 0; i < ncams - width.size(); ++i)
	{
		width.push_back(0);
	}

	for (auto i = 0; i < ncams - fourcc.size(); ++i)
	{
		fourcc.push_back("");
	}
}

string filePrefix()
{
	auto now = pt::second_clock::local_time();
	stringstream ts;
	ts << now.date().year() << "_"
		<< static_cast<int>(now.date().month()) << "_"
		<< now.date().day() << "-at-"
		<< now.time_of_day().hours() << "_"
		<< now.time_of_day().minutes();

	string file_prefix_string(ts.str());
	return file_prefix_string;
}

vector<string> makeDirectory(string& file_prefix, string& root_path)
{
	string folder_name = {root_path + file_prefix};

	const fs::path boostPath(folder_name);

	try
	{
		create_directories(boostPath);
	}
	catch (const fs::filesystem_error& e)
	{
		cerr << "Error: " << e.what() << endl;
	}

	vector<string> paths;
	paths.push_back(folder_name);
	paths.push_back(file_prefix);

	return paths;
}

void mergeMatVectors(vector<vector<Mat>>& v0, vector<vector<Mat>>& v1, vector<vector<Mat>>& v2)
{
	int last_length = 3;
	for (auto i = 0; i < v0.size(); i++)
	{
		v0[i].clear();
		auto size_1 = v1[i].size();
		auto size_2 = v2[i].size();

		for (auto j = size_1 - 1; j > size_1 - last_length && j >= 0; --j)
		{
			v0[i].emplace_back(v1[i][j]);
		}
		for (auto k = 0; k < size_2; ++k)
		{
			v0[i].emplace_back(v2[i][k]);
		}
	}
}

void mergeTimestamps(vector<vector<int>>& v0, vector<vector<int>>& v1, vector<vector<int>>& v2)
{
	int last_length = 3;
	for (auto i = 0; i < v0.size(); i++)
	{
		v0[i].clear();
		auto size_1 = v1[i].size();
		auto size_2 = v2[i].size();

		for (auto j = size_1 - 1; j > size_1 - last_length && j >= 0; --j)
		{
			v0[i].emplace_back(v1[i][j]);
		}
		for (auto k = 0; k < size_2; ++k)
		{
			v0[i].emplace_back(v2[i][k]);
		}
	}
}

int msTimestamp(time_point<high_resolution_clock>& tick)
{
	time_point<high_resolution_clock> tock = high_resolution_clock::now();
	duration<double> tdur = tock - tick;
	milliseconds ms = duration_cast<milliseconds>(tdur);
	return static_cast<int>(ms.count());
}

void printTime(string text, time_point<high_resolution_clock> ts)
{
	cout << text << ": " << msTimestamp(ts) << endl;
}

void recSwitch(int recPosition, void*)
{
	cout << "Recording: " << recPosition << endl;
}

void removeDirectory(string& folder)
{
	const fs::path boostPath(folder);

	try
	{
		uintmax_t nr = remove_all(boostPath);
	}
	catch (const fs::filesystem_error& e)
	{
		cerr << "Error: " << e.what() << endl;
	}
}

void videoDisplaySetup(Size& grid, Size& imdim, vector<Mat>& imgs, double& scaler, int c_row, int c_col)
{
	// find number of images for placement
	int n_imgs = static_cast<int>(imgs.size());
	int n_cols;
	int m_rows;

	// calculate m rows and n cols based on num images
	if ((c_row == -1 || c_col == -1) || (c_row * c_col < n_imgs))
	{
		n_cols = static_cast<int>(ceil(sqrt(n_imgs)));
		m_rows = static_cast<int>(ceil(static_cast<double>(n_imgs) / static_cast<double>(n_cols)));
	}
	else
	{
		n_cols = c_col;
		m_rows = c_row;
	}

	// small matrix which holds x,y dims for later max function
	Mat tmp_heights = Mat_<double>(m_rows, n_cols);
	Mat tmp_widths = Mat_<double>(m_rows, n_cols);

	int dev_idx = 0;

	for (int m = 0; m < m_rows; m++)
	{
		for (int n = 0; n < n_cols; n++)
		{
			if (dev_idx < n_imgs)
			{
				// place x,y size of current image in matrix
				tmp_heights.at<double>(m, n) = imgs[dev_idx].rows * scaler;
				tmp_widths.at<double>(m, n) = imgs[dev_idx].cols * scaler;
			}
			else
			{
				tmp_heights.at<double>(m, n) = 0;
				tmp_widths.at<double>(m, n) = 0;
			}
			dev_idx++;
		}
	}

	// find mat size for each dimension
	double img_height = 0;
	double img_width;

	// max height for each row in grid
	vector<double> max_row(m_rows, 0);
	for (int m = 0; m < m_rows; m++)
	{
		minMaxIdx(tmp_heights.row(m), nullptr, &max_row[m]);
		img_height += max_row[m];
	}

	// sum of widths for each row in grid, find max
	vector<double> row_sums(m_rows, 0);
	for (int m = 0; m < m_rows; m++)
	{
		for (int n = 0; n < n_cols; n++)
		{
			row_sums[m] += tmp_widths.at<double>(m, n);
		}
	}
	auto max_col_it = max_element(std::begin(row_sums), std::end(row_sums));
	img_width = *max_col_it;

	imdim = Size(static_cast<int>(ceil(img_width + n_cols - 1)), static_cast<int>(ceil(img_height + m_rows - 1)));
	grid = Size(n_cols, m_rows);

	cout << "Display dim: "
		<< imdim.width << " x " << imdim.height
		<< "\nGrid size: "
		<< grid.width << " x " << grid.height
		<< endl;
}

void waitMilliseconds(int t)
{
	milliseconds ms(t);
	this_thread::sleep_for(ms);
}

void writeCSVHeaders(ofstream& file, vector<string> name)
{
	size_t n = name.size();
	for (int i = 0; i < n; i++)
	{
		file << name[i];
		if (i < n - 1)
		{
			file << ",";
		}
	}
	file << endl;
}

void writeTimeStampData(ofstream& file, vector<double>& timestamps)
{
	for (int i = 0; i < timestamps.size(); i++)
	{
		file << timestamps[i];
		if (i < timestamps.size() - 1)
		{
			file << ",";
		}
	}

	file << endl;
}


//****************************************************
// MAIN
//****************************************************

int main(int argc, char** argv);

int main(int argc, char** argv)
{
	// handle program options
	string config_file;
	vector<int> usb_idx;
	vector<string> ip_url;
	string save_path;
	double fps;
	int fbl;
	double resize_disp_value;
	int c_row;
	int c_col;
	string window_name;
	string four_cc;
	string file_extension;
	string common_prefix;
	vector<int> aud_idx;
	unsigned sample_rate;
	unsigned audio_channels;
	unsigned audio_buffer;
	int audio_bit_depth = 16;
	vector<double> pram_set_fps;
	vector<double> pram_set_height;
	vector<double> pram_set_width;
	vector<string> pram_set_fourcc;

	try
	{
		po::options_description generic_ops("Generic Options");
		generic_ops.add_options()
			("help", "Show help for usage of application options")
			("config,c", po::value<string>(&config_file)->default_value("cam_opts.cfg"),
			 "CONFIG FILE:\n-Path to location of a configuration (.cfg) file. "
			 "You can set common usage parameters in this file. Uses cam_opts.cfg by default.");

		po::options_description config_ops("Commande Line or Configuration File Options");
		config_ops.add_options()
			(
				"usb,u",
				po::value<vector<int>>(),
				"USB ID: Integer value of the USB camera index. "
				"Different APIs are in increments of 100. "
				"May be used multiple times."
				"\n\nExample: --usb=0 or -u 1\n\n"
			)
			(
				"url,i", po::value<vector<string>>(),
				"URL ID: String of the full url for IP cameras, no quotes. "
				"May be used multiple times."
				"\n\nExample: --url=http://112.0.0.1 or -i http://...\n\n"
			)
			(
				"out,o", po::value<string>(&save_path)->default_value("data/"),
				"OUT PATH: Path to location for where to save the output files."
				"\n\nExample: --out=data/ or -o data/\n\n"
			)
			(
				"cpx", po::value<string>(&common_prefix)->default_value(filePrefix()),
				"COMMON FILE PREFIX: Typically a date and timestamp string."
				"\n\nExample: --cpx=2015_01_13\n\n"
			)
			(
				"fps,f", po::value<double>(&fps)->default_value(25),
				"FRAMES PER SECOND: Frames per second for all output videos, "
				" no matter the input fps."
				"\n\nExample: --fps=25 or -f 25\n\n"
			)
			(
				"fbl,b", po::value<int>(&fbl)->default_value(12),
				"FRAME BUFFER LENGTH: Buffer size for holding consecutive "
				"frames. This will affect imshow latency."
				"\n\nExample: --fbl=10 or -b 10\n\n"
			)
			(
				"res,r", po::value<double>(&resize_disp_value)->default_value(0.5),
				"RESIZE SCALER: Value corresponding to how much to resize "
				"the display image."
				"\n\nExample: --res=.5 or -r .5\n\n"
			)
			(
				"rows", po::value<int>(&c_row)->default_value(-1),
				"CUSTOM ROWS: Number of rows for output display."
				"\n\nExample: --rows=2\n\n"
			)
			(
				"cols", po::value<int>(&c_col)->default_value(-1),
				"CUSTOM COLUMNS: Number of cols for output display."
				"\n\nExample: --cols=1\n\n"
			)
			(
				"win,w", po::value<string>(&window_name)->default_value("YOSHIDA-VIEW"),
				"WINDOW NAME: Name to display at top of window."
				"\n\nExample: --win=stuff or -w stuff\n\n"
			)
			(
				"codec", po::value<string>(&four_cc)->default_value("MJPG"),
				"CODEC: Upper case four letter code for the codec "
				"used to export video."
				"\n\nExample: --codec=MP4V\n\n"
			)
			(
				"ext", po::value<string>(&file_extension)->default_value("avi"),
				"EXTENSION: Lower case three letter extension/wrapper "
				"for the video file."
				"\n\nExample: --ext=m4v\n\n"
			)
			(
				"aud,a", po::value<vector<int>>(),
				"AUDIO ID: Integer value of the AUDIO device index. "
				"May be used multiple times."
				"\n\nExamples: --aud=0 or -a 1\n\n"
			)
			(
				"srate", po::value<unsigned>(&sample_rate)->default_value(48000),
				"AUDIO SAMPLE RATE: Sample rate for all audio input devices."
				"\n\nExample: -srate=48000\n\n"
			)
			(
				"channels", po::value<unsigned>(&audio_channels)->default_value(2),
				"AUDIO CHANNELS: Number of channels for each audio input device."
				"\n\nExample: -channels=2\n\n"
			)
			(
				"abuffer", po::value<unsigned>(&audio_buffer)->default_value(256),
				"AUDIO BUFFER: Audio buffer size."
				"\n\nExample: -abuffer=512\n\n"
			)
			("CAP_PROP_FPS", po::value<vector<double>>(),
			 "cap frames per second")
			("CAP_PROP_FRAME_HEIGHT", po::value<vector<double>>(),
			 "cap frame height")
			("CAP_PROP_FRAME_WIDTH", po::value<vector<double>>(),
			 "cap frame width")
			("CAP_PROP_FOURCC", po::value<vector<string>>(),
			 "cap fourcc");

		po::options_description cmdline_options;
		cmdline_options.add(generic_ops).add(config_ops);

		po::options_description config_file_options;
		config_file_options.add(config_ops);

		po::positional_options_description p;
		p.add("usb", -1);

		po::variables_map vm;
		store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
		notify(vm);

		ifstream ifs(config_file.c_str());

		if (!ifs)
		{
			cout << "No Config file found. This means you can only pass options through the command line" << endl;
		}
		else
		{
			store(parse_config_file(ifs, config_file_options), vm);
			notify(vm);
		}

		if (vm.count("help"))
		{
			cout << "Help documentation for Cogdev Lab Cam Sync:\nAuthor: Joseph M. Burling\n";
			cout << cmdline_options;
			return 0;
		}

		if (vm.count("usb"))
		{
			usb_idx = vm["usb"].as<vector<int>>();
		}

		if (vm.count("url"))
		{
			ip_url = vm["url"].as<vector<string>>();
		}

		if (vm.count("aud"))
		{
			aud_idx = vm["aud"].as<vector<int>>();
		}

		if (vm.count("CAP_PROP_FPS"))
		{
			pram_set_fps = vm["CAP_PROP_FPS"].as<vector<double>>();
		}

		if (vm.count("CAP_PROP_FRAME_HEIGHT"))
		{
			pram_set_height = vm["CAP_PROP_FRAME_HEIGHT"].as<vector<double>>();
		}

		if (vm.count("CAP_PROP_FRAME_WIDTH"))
		{
			pram_set_width = vm["CAP_PROP_FRAME_WIDTH"].as<vector<double>>();
		}

		if (vm.count("CAP_PROP_FOURCC"))
		{
			pram_set_fourcc = vm["CAP_PROP_FOURCC"].as<vector<string>>();
		}

		if (four_cc.size() != 4)
		{
			cout << "Please enter a codec with at least four characeters. see FOURCC.org." << endl;
			return 0;
		}
	}
	catch (std::exception& e)
	{
		cout << e.what() << "\n";
		return 1;
	}

	// collect number if input cameras
	auto n_camera_devices = static_cast<int>(usb_idx.size() + ip_url.size());
	if (n_camera_devices == 0)
	{
		cout << "No camera ids or urls entered. Ending program." << endl;
		return 0;
	}

	// pram struct
	fillCapSetOpts(static_cast<int>(usb_idx.size()),
	               pram_set_fps, pram_set_height, pram_set_width, pram_set_fourcc);
	CaptureParameters set_cap_parameters;
	set_cap_parameters.fps = pram_set_fps;
	set_cap_parameters.height = pram_set_height;
	set_cap_parameters.width = pram_set_width;
	set_cap_parameters.fourcc = pram_set_fourcc;

	// some additional parameters
	double frame_duration = 1000.0 / fps;
	double read_duration_ms = frame_duration * fbl;
	double actual_read_time_ms = read_duration_ms;
	double past_ts = 0.0;
	int main_thread_timestamp;
	double loop_break_time;
	int image_display_idx;
	int REC_SLIDER = 0;
	bool REC_INIT = false;
	bool START_VIDEO_WRITE = false;
	int exit_key = 27;
	Size grid_size_disp;
	Size disp_size_disp;
	auto initBufferSize = 1;
	double last_frame_timestamps;

	// audio objects
	int n_audio_devices = static_cast<int>(aud_idx.size());
	bool REC_AUDIO = false;
	if (n_audio_devices > 0) REC_AUDIO = true;
	vector<RtAudio> audio(n_audio_devices);
	vector<AudioStruct> audio_parameters(n_audio_devices);
	vector<RtAudio::StreamParameters> stream_parameters(n_audio_devices);

	// initialize cameras, get first frames
	vector<Mat> images_to_show(n_camera_devices);
	vector<VideoCapture> capVec = initCapDevices(usb_idx, ip_url, images_to_show, set_cap_parameters);
	printCapParameters(capVec);
	videoDisplaySetup(grid_size_disp, disp_size_disp, images_to_show, resize_disp_value, c_row, c_col);
	namedWindow(window_name, WINDOW_AUTOSIZE);
	createTrackbar("REC", window_name, &REC_SLIDER, 1, recSwitch, nullptr);

	while (true)
	{
		for (int i = 0; i < n_camera_devices; i++)
		{
			capVec[i] >> images_to_show[i];
		}

		imDisplay(window_name, images_to_show, disp_size_disp,
		          grid_size_disp, resize_disp_value, REC_SLIDER);

		if (REC_SLIDER == 1)
		{
			REC_INIT = true;
			break;
		}

		if (waitKey(static_cast<int>(frame_duration)) == exit_key)
		{
			break;
		}
	}

	if (REC_INIT)
	{
		// make output directory
		vector<string> file_str = makeDirectory(common_prefix, save_path);

		// initialize writer
		vector<VideoWriter> writeVec = initWriteDevices(capVec, file_str,
		                                                fps, four_cc, file_extension);

		// cam data file
		ofstream cam_ts(file_str[0] + "/cameraTimeStamps_0_" + file_str[1] + ".csv");
		vector<string> cam_headers{"clock"};
		for (int i = 0; i < usb_idx.size(); ++i)
		{
			cam_headers.push_back("usb_ts_" + to_string(usb_idx[i]));
		}
		for (int i = 0; i < ip_url.size(); ++i)
		{
			cam_headers.push_back("url_ts_" + to_string(i));
		}
		for (int i = 1; i < n_camera_devices + 1; ++i)
		{
			cam_headers[i] = cam_headers[i] + "_" + to_string(i - 1);
		}
		writeCSVHeaders(cam_ts, cam_headers);

		// audio timestamp files
		vector<ofstream> aud_ts(n_audio_devices);
		for (int i = 0; i < n_audio_devices; ++i)
		{
			vector<string> aud_headers{"clock"};
			aud_headers.push_back("aud_ts_" + to_string(aud_idx[i]) + "_" + to_string(i));
			aud_ts[i].open(file_str[0] + "/audioTimeStamps_" +
				to_string(i) + "_" + file_str[1] + ".csv");
			writeCSVHeaders(aud_ts[i], aud_headers);
		}

		// make frame and timestamp buffers
		vector<vector<Mat>> pastBuffer;
		vector<vector<int>> pastTimestamps;
		vector<vector<Mat>> extendedBuffer(n_camera_devices);
		vector<vector<int>> extendedTimestamps(n_camera_devices);
		vector<vector<Mat>> presentBuffer(n_camera_devices);
		vector<vector<int>> presentTimestamps(n_camera_devices);

		// init main clock
		time_point<high_resolution_clock> startTime;

		// fill the past buffer before main loop
		for (int i = 0; i < initBufferSize; i++)
		{
			for (int j = 0; j < n_camera_devices; j++)
			{
				Mat tempImg;
				capVec[j] >> tempImg;
				colorizeMat(tempImg);
				extendedTimestamps[j].emplace_back(0);
				extendedBuffer[j].emplace_back(tempImg);
			}
		}

		//initialize audio devices
		if (REC_AUDIO)
		{
			// check if audio input devices exist
			unsigned int n_devices = audio[0].getDeviceCount();
			cout << "\nNumber of Audio Devices: " << n_devices - 1 << endl;

			if (n_devices < 1)
			{
				cout << "\nNo audio input devices found!\n";
				return 0;
			}

			// load audio device parameters
			audioDeviceParameters(audio_parameters, stream_parameters,
			                      aud_idx, sample_rate, audio_channels,
			                      audio_buffer, file_str);

			// try to open audio devices
			for (int i = 0; i < n_audio_devices; ++i)
			{
				audioHeader(audio_parameters[i].file, audio_bit_depth, audio_channels, sample_rate);
				try
				{
					audio[i].openStream(nullptr,
					                    &stream_parameters[i],
					                    RTAUDIO_SINT16,
					                    audio_parameters[i].samplerate,
					                    &audio_parameters[i].buffersize,
					                    &audioWriter, static_cast<void *>(&audio_parameters[i]));
					cout << "Audio stream latency "
						<< to_string(aud_idx[i]) << ": "
						<< audio[0].getStreamLatency()
						<< " frames" << endl;
					audio[i].setStreamTime(0);
				}
				catch (RtAudioError& e)
				{
					e.printMessage();
					return 0;
				}
			}
		}

		// start main clock
		startTime = high_resolution_clock::now();

		// start looping through main thread
		while (true)
		{
			// hold frames and timestamps for next loop iteration
			vector<future<void>> presentBufferThread(n_camera_devices);
			main_thread_timestamp = msTimestamp(startTime); // main start timestamp
			loop_break_time = static_cast<double>(main_thread_timestamp) + read_duration_ms + 2 - (frame_duration / 2);

			// Start threaded buffer fill until reach end time
			for (auto i = 0; i < n_camera_devices; i++)
			{
				presentBufferThread[i] = async(
					launch::async,
					[
						&capVec,
						&presentBuffer,
						&startTime,
						&presentTimestamps,
						&loop_break_time
					]
					(int j)
					{
						presentTimestamps[j].clear();
						presentBuffer[j].clear();
						double currentLoopTime;

						while (true)
						{
							// grab, timestamp, decode, push to buffer, check time
							Mat placeHolderImg;
							capVec[j].grab();
							presentTimestamps[j].emplace_back(msTimestamp(startTime));
							capVec[j].retrieve(placeHolderImg);
							presentBuffer[j].emplace_back(placeHolderImg.clone());
							currentLoopTime = static_cast<double>(msTimestamp(startTime));
							if (currentLoopTime > loop_break_time)
							{
								break;
							}
						}
					}, i);
			}

			// first slot is the common time stamp, loop time
			vector<double> collectedCamTs(n_camera_devices + 1);
			vector<future<void>> audio_ts_thread(n_audio_devices);
			last_frame_timestamps = 0.0;
			image_display_idx = 0;
			pastTimestamps = extendedTimestamps;
			pastBuffer = extendedBuffer;

			if (REC_SLIDER == 1)
			{ // if 0 then pause recording

				if (REC_AUDIO)
				{ // deal with audio data timestamps
					for (auto i = 0; i < n_audio_devices; ++i)
					{
						// async timestamp collection
						audio_ts_thread[i] = async(
							launch::async,
							[
								&audio,
								&aud_ts,
								&frame_duration,
								&startTime,
								&loop_break_time
							]
							(int j)
							{
								// start stream if not running
								if (audio[j].isStreamOpen() && !audio[j].isStreamRunning())
								{
									try
									{
										audio[j].startStream();
										audio[j].setStreamTime(0);
									}
									catch (RtAudioError& e)
									{
										e.printMessage();
									}
								}
								while (true)
								{
									waitMilliseconds(static_cast<int>(round(frame_duration)));
									vector<double> ts_dat(2);
									ts_dat[0] = static_cast<double>(msTimestamp(startTime));
									ts_dat[1] = audio[j].getStreamTime() * 1000;
									if (static_cast<double>(ts_dat[0]) >= loop_break_time - frame_duration + 1) break;
									writeTimeStampData(aud_ts[j], ts_dat);
								}
							}, i);
					}
				}

				// process previous frames using timestamps
				if (START_VIDEO_WRITE)
				{
					for (double t = past_ts;
					     t < actual_read_time_ms;
					     t = t + frame_duration)
					{
						collectedCamTs[0] = t;
						vector<future<void>> readWriteBuffer(n_camera_devices);

						// write asynchronously
						for (auto i = 0; i < n_camera_devices; i++)
						{
							readWriteBuffer[i] = async(
								launch::async,
								[
									&pastBuffer,
									&pastTimestamps,
									&collectedCamTs,
									&t,
									&writeVec,
									&images_to_show,
									&image_display_idx
								]
								(int j)
								{
									auto nearestTimeStamp = findNearestTimeStamp(t, pastTimestamps[j]);
									collectedCamTs[j + 1] = static_cast<double>(pastTimestamps[j][nearestTimeStamp]);
									writeVec[j] << pastBuffer[j][nearestTimeStamp];
									if (image_display_idx == 0)
									{
										images_to_show[j] = pastBuffer[j].back();
									}
								}, i);
						}

						// wait for all devices to write one frame
						for (int i = 0; i < n_camera_devices; i++)
						{
							readWriteBuffer[i].wait();
						}

						// write timestamp data
						writeTimeStampData(cam_ts, collectedCamTs);
						image_display_idx++; // for copying only the last image to view
					}
				}
			}
			else // pause writing
			{
				// just make viewing images only
				for (int i = 0; i < n_camera_devices; i++)
				{
					images_to_show[i] = pastBuffer[i].back();
				}

				// stop all audio streams at once
				if (REC_AUDIO)
				{
					vector<future<void>> audio_pause_thread;
					for (auto i = 0; i < n_audio_devices; ++i)
					{
						audio_pause_thread.emplace_back(
							async(launch::async, [&audio](int j)
							      {
								      // pause stream
								      if (audio[j].isStreamOpen() && audio[j].isStreamRunning())
								      {
									      audio[j].stopStream();
								      }
							      }, i));
					}
					for (int i = 0; i < n_audio_devices; ++i)
					{
						audio_pause_thread[i].wait();
					}
				}
			}

			// show last image from buffer
			imDisplay(window_name, images_to_show, disp_size_disp,
			          grid_size_disp, resize_disp_value, REC_SLIDER);

			if (waitKey(1) == exit_key)
			{
				destroyWindow(window_name);
				for (auto i = 0; i < n_camera_devices; i++)
				{
					presentBufferThread[i].wait();
				}
				break;
			}

			// wait for audio timestamp writers
			if (REC_AUDIO)
			{
				for (auto i = 0; i < audio_ts_thread.size(); ++i)
				{
					audio_ts_thread[i].wait();
				}
			}

			// wait for read threads to finish capturing images
			for (int i = 0; i < presentBufferThread.size(); i++)
			{
				presentBufferThread[i].wait();
				last_frame_timestamps = last_frame_timestamps +
					static_cast<double>(presentTimestamps[i].back());
			}

			// average end time from all cameras
			actual_read_time_ms = last_frame_timestamps /
				static_cast<double>(presentTimestamps.size());

			// start time for next write loop
			past_ts = collectedCamTs[0] + frame_duration;

			// combine partial old with complete new
			mergeMatVectors(extendedBuffer, pastBuffer, presentBuffer);
			mergeTimestamps(extendedTimestamps, pastTimestamps, presentTimestamps);

			START_VIDEO_WRITE = true;
		}

		// close audio devices and files
		if (REC_AUDIO)
		{
			for (auto i = 0; i < aud_idx.size(); ++i)
			{
				aud_ts[i].close();
				if (audio[i].isStreamOpen())
				{
					if (audio[i].isStreamRunning()) audio[i].stopStream();
					audio[i].closeStream();
					audioDataSize(audio_parameters[i].file); // write final data size to header
					audio_parameters[i].file.close();
				}
			}
		}

		// release video writer devices
		for (auto i = 0; i < capVec.size(); i++)
		{
			writeVec[i].release();
		}

		// close data file
		cam_ts.close();
	}

	// release video capture devices
	for (int i = 0; i < capVec.size(); i++)
	{
		capVec[i].release();
	}

	// try to rm empty dir if not recording
	//    if (REC_FLAG == false) {
	//        cout << "Removing Temp Folder: " << file_str[0] << endl;
	//        waitMilliseconds(1000);
	//        removeDirectory(file_str[0]);
	//    }

	return 0;
}
