% NLMS_SIMULATION  Adaptive noise cancelling test for the cable fault locator.
%
% Two cases:
%   1. Synthetic: a sine wave buried in white Gaussian noise.
%   2. Recorded:  your own primary and reference WAV files (optional).
%
% The filter is written out in full, so no toolbox is needed.
%
%   y(n)   = w' * x(n)                         noise estimate
%   e(n)   = d(n) - y(n)                       cleaned signal
%   w(n+1) = w(n) + mu * e(n) * x(n) / (eps + x(n)' * x(n))
%
% d(n) is the primary mic (thump + noise). x(n) is the reference mic (noise).
% The same equations run on the STM32 in firmware/stm32f401re-nlms/Core/Src/nlms.c.

clear; close all; clc;

%% Parameters
fs     = 16000;   % sample rate [Hz], same as the firmware
T      = 3;       % signal length [s]
f_sig  = 150;     % test tone [Hz], in the low band where thumps carry most energy
taps   = 32;      % filter length, same as the firmware
mu     = 0.05;    % step size, 0 < mu < 2
eps_r  = 1e-6;    % regularization

%% Case 1: sine wave in white Gaussian noise
t   = (0:1/fs:T - 1/fs).';
s   = 0.05 * sin(2*pi*f_sig*t);           % wanted signal [normalized amplitude]
ref = 0.1  * randn(size(t));              % noise at the reference mic
h   = [0 0.6 -0.3 0.15 0.05].';           % unknown path from noise source to primary mic
d   = s + filter(h, 1, ref);              % primary mic = signal + filtered noise

e = nlms(d, ref, taps, mu, eps_r);

fprintf('Case 1: SNR in = %.1f dB, SNR out (last 1 s) = %.1f dB\n', ...
    snr_db(s, d - s), snr_db(s(end-fs+1:end), e(end-fs+1:end) - s(end-fs+1:end)));

figure('Name', 'Case 1: sine in white noise');
subplot(3,1,1); plot(t, s);  title('Wanted signal');        ylabel('Amplitude');
subplot(3,1,2); plot(t, d);  title('Primary mic (noisy)');  ylabel('Amplitude');
subplot(3,1,3); plot(t, e);  title('NLMS output');          ylabel('Amplitude'); xlabel('Time [s]');

%% Case 2: recorded audio (optional)
% Record both mics at the same time and save them as mono WAV files.
primary_file   = 'primary.wav';
reference_file = 'reference.wav';

if isfile(primary_file) && isfile(reference_file)
    [d2, fs_d] = audioread(primary_file);
    [x2, fs_x] = audioread(reference_file);
    assert(fs_d == fs_x, 'Both recordings must use the same sample rate.');
    n  = min(numel(d2), numel(x2));
    e2 = nlms(d2(1:n, 1), x2(1:n, 1), taps, mu, eps_r);
    t2 = (0:n-1).' / fs_d;

    figure('Name', 'Case 2: recorded audio');
    subplot(2,1,1); plot(t2, d2(1:n, 1)); title('Primary mic'); ylabel('Amplitude');
    subplot(2,1,2); plot(t2, e2);         title('NLMS output'); ylabel('Amplitude'); xlabel('Time [s]');

    audiowrite('cleaned.wav', e2 / max(abs(e2) + eps), fs_d);
    fprintf('Case 2: wrote cleaned.wav\n');
else
    fprintf('Case 2 skipped: put primary.wav and reference.wav next to this script.\n');
end

%% Local functions
function e = nlms(d, x, taps, mu, eps_r)
% NLMS  Normalized LMS adaptive noise canceller. Returns the error signal.
    w   = zeros(taps, 1);
    buf = zeros(taps, 1);
    e   = zeros(size(d));
    for n = 1:numel(d)
        buf  = [x(n); buf(1:end-1)];
        y    = w.' * buf;
        e(n) = d(n) - y;
        w    = w + (mu * e(n) / (eps_r + buf.' * buf)) * buf;
    end
end

function r = snr_db(sig, noise)
% SNR_DB  Signal-to-noise ratio in decibels.
    r = 10 * log10(sum(sig.^2) / sum(noise.^2));
end
