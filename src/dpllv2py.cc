#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "peaklim.cc"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

class PeaklimWithPythonExtensions
{
  public:
    PeaklimWithPythonExtensions() 
    : m_peaklim(new DPLLV2::Peaklim())
    {
    }

    ~PeaklimWithPythonExtensions()
    {
        delete m_peaklim;
    }

    void initialise(float samplerate, int numchannels)
    {
        m_numchannels = numchannels;
        m_peaklim->init(samplerate, numchannels);
    }

    void set_input_gain(float gain)
    {
        m_peaklim->set_inpgain(gain);
    }

	void set_threshold(float threshold)
    {
        m_peaklim->set_threshold(threshold);
    }

	void set_release(float release)
    {
        m_peaklim->set_release(release);
    }

	void set_truepeak(bool truepeakon)
    {
        m_peaklim->set_truepeak(truepeakon);
    }

    int get_latency() const
	{
		return m_peaklim->get_latency();
	}

    void process_py(const py::array_t<float> &input, py::array_t<float> &output)
    {
        auto in_buf = input.request();
        auto out_buf = output.request(true);

        if (in_buf.itemsize != sizeof(float))
        {
            std::ostringstream oss;
            oss << "Input numpy array must have dtype float; you provided an array with "
                << in_buf.format << "/" << in_buf.size;
            throw std::invalid_argument(oss.str().c_str());
        }

        if (out_buf.itemsize != sizeof(float))
        {
            std::ostringstream oss;
            oss << "Output numpy array must have dtype float; you provided an array with "
                << out_buf.format << "/" << out_buf.size;
            throw std::invalid_argument(oss.str().c_str());
        }

        if (out_buf.ndim != 2)
        {
            std::ostringstream oss;
            oss << "Output numpy array must have 2 dimensions (2, numframes); you provided an "
                   "array with "
                << out_buf.ndim << " dimensions";
            throw std::invalid_argument(oss.str().c_str());
        }

        if (out_buf.shape[0] != 2)
        {
            std::ostringstream oss;
            oss << "Output numpy array must have dimensions (2, numframes); you provided an "
                   "array with "
                << out_buf.shape[0] << "x" << out_buf.shape[1];
            throw std::invalid_argument(oss.str().c_str());
        }

        if (in_buf.shape != out_buf.shape)
        {
            std::ostringstream oss;
            oss << "Input numpy array dimensions must match output; you provided an "
                   "array with "
                << in_buf.shape[0] << "x" << in_buf.shape[1];
            throw std::invalid_argument(oss.str().c_str());
        }

        auto in_ptr = static_cast<float*>(in_buf.ptr);
        auto out_ptr = static_cast<float*>(out_buf.ptr);
        const auto numframes = out_buf.shape[1]; 

        float* inputs[2] = {in_ptr, in_ptr};
        float* outputs[2] = {out_ptr, out_ptr};

        if (m_numchannels == 2)
        {
            inputs[1] = in_ptr + numframes;
            outputs[1] = out_ptr + numframes;
        }

        m_peaklim->process(numframes, inputs, outputs);
        m_peaklim->get_stats(&peak, &gmax, &gmin);
    }

    float peak;
    float gmax;
    float gmin;

private:
    DPLLV2::Peaklim* m_peaklim;
    int m_numchannels;
};

PeaklimWithPythonExtensions *createPeaklim(float samplerate, int numchannels)
{
    auto peaklim = new PeaklimWithPythonExtensions();
    peaklim->initialise(samplerate, numchannels);
    return peaklim;
}

PYBIND11_MODULE(dpllv2, m)
{
    m.doc() = "Python bindings for DPL LV2";
    m.def("createPeaklim", &createPeaklim, "Create a DPL LV2 instance", py::arg("samplerate"), py::arg("numchannels"));

    py::class_<PeaklimWithPythonExtensions>(m, "Peaklim")
        .def(pybind11::init())
        .def("initialise",
                &PeaklimWithPythonExtensions::initialise,
                "Initialise",
                py::arg("samplerate"), py::arg("numchannels"))

        .def("set_input_gain",
             &PeaklimWithPythonExtensions::set_input_gain,
             "Set input gain",
             py::arg("gain"))

        .def("set_threshold",
             &PeaklimWithPythonExtensions::set_threshold,
             "Set threshold",
             py::arg("threshold"))

        .def("set_release",
             &PeaklimWithPythonExtensions::set_release,
             "Set release",
             py::arg("release"))

        .def("set_truepeak",
             &PeaklimWithPythonExtensions::set_truepeak,
             "Set truepeak",
             py::arg("truepeak"))

        .def("get_latency",
             &PeaklimWithPythonExtensions::get_latency,
             "Get latency")

        .def_readwrite("peak", &PeaklimWithPythonExtensions::peak)
        .def_readwrite("gmax", &PeaklimWithPythonExtensions::gmax)
        .def_readwrite("gmin", &PeaklimWithPythonExtensions::gmin)

        .def("process",
             &PeaklimWithPythonExtensions::process_py,
             "Process input array and fill output array",
             py::arg("input").noconvert(true), py::arg("output").noconvert(true));

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
