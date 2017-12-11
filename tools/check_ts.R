library(mejr) # https://github.com/iamamutt/mejr
library(data.table)
library(ggplot2)

SAMP_RATE <- 48000
PULSE_RATE <- 1000

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
    fps <- 1000 / pulse_interval
    pct_err <- 100 * (sum(err) / n)
    time_sec <- (max(ts) - min(ts)) / 1000
    data.table(n, time_sec, pulse_interval, fps, pct_err, ms_mean, ms_sd)
}

error_info <- function(err) {
    n <- length(err)
    data.table(
        rec_error = 100 * (sum(err == -1) / n),
        ply_error = 100 * (sum(err == -2) / n)
    )
}

get_ts <- function(dt) {
    dt[status == 0, ][order(master_time)][buffer > 0]
}

check_ts <- function(dt) {
    ts_data <- get_ts(dt)
    stream_d <- ts_data[, stream_info(stream_time)]
    master_d <- ts_data[, stream_info(master_time)]
    stream_d[, stream := "rtaudio"]
    master_d[, stream := "program"]

    list(
        streams = rbind(stream_d, master_d),
        errors = error_info(dt$status)
    )
}

plot_ts <- function(dt, fps=PULSE_RATE) {
    ts_data <- dt[status >= 0,][order(expected, master_time)][buffer > 0]
    ts_cols <- c("stream_time", "expected", "master_time")
    min_t <- min(ts_data[, lapply(.SD, min), .SDcols = ts_cols])
    ts_data[, eval(ts_cols) := lapply(.SD, function(i) i - min_t), .SDcols = ts_cols]
    ts_data[, samp := 1000 * ((buffer - 1) * size / SAMP_RATE)]
    ts_data[, samp := samp + (master_time - min(master_time)), buffer]

    bsize <- ts_data[, median(size)]
    cf <- coef(lm(stream_time ~ expected, data=ts_data))

    ggplot(ts_data)+
        aes(x=expected)+
        # geom_line(aes(y=expected))+
        # geom_line(aes(y=stream_time))+
        geom_line(aes(y=master_time),
                  color = 3)+
        geom_abline(intercept = 0, #min(ts_data$expected),
                    slope = 2,
                    color = gray(0.5),
                    size = 1,
                    linetype = 3)

    i <- 1000 / fps
    ts_data[, seq(min(expected), max(master_time) * 2, i)[1:.N]]
    fps_ts <- seq(0, max(dt$expected)+i, i)
    plot(fps_ts, fps_ts, type = "l", col = gray(0.8), lwd = 10, xlab = "Expected ts", ylab = "Captured ts")
    lines(dt$expected, dt$stream_time, col = 2, lwd = 1)
    lines(dt$expected, dt$master_time, col = 3, lwd = 1)

    lines(dt$expected, dt$expected, col = 0, lty = 2)
}

dt <- read_ts_file()
ts_info <- check_ts(dt)
print(ts_info$streams)
print(ts_info$errors)
#plot_ts(dt)
write.table(dt[status == 0, .(stream_time)],
            "timestamps.txt",
            row.names = FALSE,
            col.names = FALSE)
