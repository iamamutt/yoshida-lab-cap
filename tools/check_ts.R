library(mejr) # https://github.com/iamamutt/mejr
library(data.table)
library(ggplot2)

SAMP_RATE <- 48000
PULSE_RATE <- 120

setwd(mejr::getcrf())

read_ts_file <- function() {
    filepath <- list.files(file.path("..", "test", "temp"),
                           pattern = "audio_ts.*\\.csv$",
                           full.names = TRUE)[1]
    dt <- data.table::fread(filepath)
    dt[order(master_time)]
}

stream_info <- function(ts) {
    delta <- diff(ts)
    ms_mean <- mean(delta)
    ms_sd <- sd(delta)
    n <- length(delta) + 1
    err <- delta > ms_mean + ms_sd * 3 | delta < ms_mean - ms_sd * 3
    pulse_interval <-  mean(delta[!err])
    fps <- 1000 / ms_mean
    pct_err <- 100 * (sum(err) / n)
    time_sec <- (max(ts) - min(ts)) / 1000
    data.table(n, time_sec, pulse_interval, fps, pct_err, ms_mean, ms_sd)
}

error_info <- function(dt) {
    n <- dt[status == 0, .N]
    rec_err <- dt[status == -1, .N]
    ply_err <- dt[status == -2, .N]
    data.table(
        rec_error = 100 * (rec_err / n),
        ply_error = 100 * (ply_err / n)
    )
}

get_ts <- function(dt) {
    dt[status == 0, ][order(master_time)][buffer > 0]
}

check_ts <- function(dt) {
    ts_data <- get_ts(dt)
    stream_d <- ts_data[,stream_info(stream_time)]
    master_d <- ts_data[,stream_info(master_time)]
    audio_d <- ts_data[,stream_info(audio_time)]
    audio_d[, stream := "wavfile"]
    stream_d[, stream := "rtaudio"]
    master_d[, stream := "program"]

    list(streams = rbind(audio_d, stream_d, master_d),
         errors = error_info(dt))
}


plot_ts <- function(dt, fps = PULSE_RATE) {
    ts_data <-
        dt[status == 0, ][order(master_time, audio_time)][buffer > 0]
    ts_data[, `:=` (
        stream_d = (min(stream_time) + (audio_time - min(audio_time))) - stream_time,
        master_d = (min(master_time) + (audio_time - min(audio_time))) - master_time
    )]

    plot(ggplot(ts_data) +
        aes(x = audio_time) +
        geom_line(
            aes(y = stream_d),
            color = "red",
            alpha = 0.5,
            size = 0.25
        ) +
        geom_line(
            aes(y = master_d),
            color = "blue",
            alpha = 0.5,
            size = 1
        ) +
        geom_abline(
            intercept = 0,
            slope = 0,
            color = gray(0.5),
            size = 0.25,
            linetype = 3
        ))
}

dt <- read_ts_file()
audio_ts <- dt[status == 0, audio_time]
ts_info <- check_ts(dt)
print(ts_info$streams)
print(ts_info$errors)
plot_ts(dt)
write.table(dt[status == 0, .(stream_time)],
            "timestamps.txt",
            row.names = FALSE,
            col.names = FALSE)
