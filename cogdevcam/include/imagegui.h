/**
    project: cogdevcam
    source file: imagegui
    description:

    @author Joseph M. Burling
    @version 0.9.0 12/10/2017
*/

#ifndef COGDEVCAM_IMAGEGUI_H
#define COGDEVCAM_IMAGEGUI_H

#include "tools.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace imagegui {
class WinShow
{
  public:
    WinShow() = default;

    void
    openWindow(const std::string &_window_name = "")
    {
        initWindow(_window_name);
    };

    bool
    isSliderSet() const
    {
        return slider_state == 1;
    };

    bool
    isWindowOpen() const
    {
        return window_open;
    };

    bool
    isDisplaySet() const
    {
        return !display_size.empty();
    };

    void
    closeWindow()
    {
        cv::destroyAllWindows();
        slider_state = 0;
        window_open  = false;
        display_size = cv::Size();
    };

    void
    videoDisplaySetup(const std::vector<cv::Mat> &image_vec,
                      int                         _cols,
                      int                         _rows  = -1,
                      double                      _scale = 1)
    {
        display_scale = _scale;

        // find number of images for placement
        auto n_images = image_vec.size();
        auto n_show   = static_cast<double>(n_images);
        int  n_cols;
        int  m_rows;

        // calculate m rows and n cols based on num images
        if (_cols == -1)
        {
            n_cols = static_cast<int>(std::ceil(std::sqrt(n_show)));
        } else
        {
            n_cols = _cols;
        }
        if (_rows == -1)
        {
            m_rows = static_cast<int>(
              std::ceil(n_show / static_cast<double>(n_cols)));
        } else
        {
            m_rows = _rows;
        }
        if (n_cols * m_rows < n_show)
        {
            n_cols = static_cast<int>(std::ceil(std::sqrt(n_show)));
            m_rows = static_cast<int>(
              std::ceil(n_show / static_cast<double>(n_cols)));
        }

        // small matrix which holds x,y dims for later max function
        cv::Mat tmp_heights = cv::Mat_<double>(m_rows, n_cols);
        cv::Mat tmp_widths  = cv::Mat_<double>(m_rows, n_cols);
        int     dev_idx     = 0;
        for (int m = 0; m < m_rows; ++m)
        {
            for (int n = 0; n < n_cols; ++n)
            {
                if (dev_idx < n_images)
                {
                    // place x,y size of current image in matrix
                    tmp_heights.at<double>(m, n) = image_vec[dev_idx].rows *
                                                   display_scale;
                    tmp_widths.at<double>(m, n) = image_vec[dev_idx].cols *
                                                  display_scale;
                } else
                {
                    tmp_heights.at<double>(m, n) = 0;
                    tmp_widths.at<double>(m, n)  = 0;
                }
                ++dev_idx;
            }
        }

        // find mat size for each dimension
        double img_height = 0;
        double img_width;

        // max height for each row in grid
        std::vector<double> max_row(static_cast<unsigned>(m_rows), 0);
        for (int m = 0; m < m_rows; ++m)
        {
            minMaxIdx(tmp_heights.row(m), nullptr, &max_row[m]);
            img_height += max_row[m];
        }

        // sum of widths for each row in grid, find max
        std::vector<double> row_sums(static_cast<unsigned>(m_rows), 0);
        for (int m = 0; m < m_rows; ++m)
        {
            for (int n = 0; n < n_cols; ++n)
            {
                row_sums[m] += tmp_widths.at<double>(m, n);
            }
        }
        auto max_col_it = std::max_element(
          std::begin(row_sums), std::end(row_sums));
        img_width    = *max_col_it;
        display_size = cv::Size(
          static_cast<int>(std::ceil(img_width + n_cols)),
          static_cast<int>(std::ceil(img_height + m_rows)));

        display_layout = cv::Size(n_cols, m_rows);
        std::cout << "\nDisplayed images frame dimensions:\n  Width="
                  << display_size.width << ", Height=" << display_size.height
                  << "\nGrid size:\n  Columns=" << display_layout.width
                  << ", Rows=" << display_layout.height << "\n";
    };

    cv::Mat
    showImages(const std::vector<cv::Mat> &image_vec)
    {
        if (!isWindowOpen()) initWindow();
        if (display_size.empty()) throw err::Runtime("Display not set");

        cv::Mat out_img = cv::Mat::zeros(
          display_size.height, display_size.width, CV_8UC3);

        auto n_images    = static_cast<int>(image_vec.size());
        int  img_idx     = 0;
        int  out_h_start = 1;
        int  out_w_start;
        int  out_h_end    = 0;
        int  out_w_end    = 0;
        bool no_more_imgs = false;
        for (auto m = 0; m < display_layout.height; ++m)
        {
            out_w_start = 1;
            for (auto n = 0; n < display_layout.width; ++n, ++img_idx)
            {
                if (img_idx < n_images)
                {
                    cv::Mat in_img;
                    resize(image_vec[img_idx],
                           in_img,
                           cv::Size(),
                           display_scale,
                           display_scale,
                           cv::INTER_NEAREST);

                    cv::Size in_img_sz  = in_img.size();
                    out_h_end           = out_h_start + (in_img_sz.height - 1);
                    out_w_end           = out_w_start + (in_img_sz.width - 1);
                    cv::Range in_roi_h  = cv::Range(0, in_img_sz.height - 1);
                    cv::Range in_roi_w  = cv::Range(0, in_img_sz.width - 1);
                    cv::Range out_roi_w = cv::Range(out_w_start, out_w_end);
                    cv::Range out_roi_h = cv::Range(out_h_start, out_h_end);

                    in_img(in_roi_h, in_roi_w)
                      .copyTo(out_img(out_roi_h, out_roi_w));
                } else
                {
                    no_more_imgs = true;
                    break;
                }
                out_w_start = out_w_end + 2;
            }
            if (no_more_imgs)
            {
                break;
            }
            out_h_start = out_h_end + 2;
        }
        if (isSliderSet())
        {
            circle(
              out_img, cv::Point(15, 15), 14, cv::Scalar(0, 0, 255), -1, 8);
            putText(out_img,
                    "REC",
                    cv::Point(30, 20),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.5,
                    cv::Scalar(255, 255, 255));
        }
        cv::imshow(window_name, out_img);
        return out_img;
    };

    void
    colorizeMat(cv::Mat &img)
    {
        for (auto m = 0; m < img.rows; ++m)
        {
            for (auto n = 0; n < img.cols; ++n)
            {
                img.at<cv::Vec3b>(m, n)[0] = 0;
                img.at<cv::Vec3b>(m, n)[1] = 0;
                img.at<cv::Vec3b>(m, n)[2] = 255;
            }
        }
    };

  private:
    std::string window_name  = "COGDEVCAM";
    int         slider_state = 0;
    bool        window_open  = false;
    cv::Size    display_layout;
    cv::Size    display_size;
    double      display_scale;

    void
    initWindow(std::string name = "")
    {
        if (!name.empty()) window_name = name;
        cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
        cv::setWindowTitle(window_name, "PREVIEW_MODE: " + window_name);
        cv::createTrackbar("REC",
                           window_name,
                           &slider_state,
                           1,
                           imagegui::WinShow::recSwitch,
                           &window_name);
        window_open = true;
    };

    static void
    recSwitch(int switch_position, void *data)
    {
        auto *win_name = static_cast<std::string *>(data);
        if (switch_position == 0)
        {
            cv::setWindowTitle(*win_name, "PREVIEW_MODE: " + *win_name);
        } else
        {
            cv::setWindowTitle(*win_name, *win_name);
        }
        std::cout << (switch_position == 0 ? "\n...Paused recording\n" :
                                             "\n...Recording started\n");
    }
};
};  // namespace imagegui

#endif  // COGDEVCAM_IMAGEGUI_H
