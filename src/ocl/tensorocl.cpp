#include <miopen/tensor.hpp>
#include <miopen/tensor_ops.hpp>
#include <miopen/errors.hpp>
#include <algorithm>
#include <numeric>

namespace miopen {

void TensorDescriptor::SetTensor(Handle& /* handle */,
		Data_t							dstTensor,
		const void						*valuePtr) {

	printf("To be implemented (SetTensor) \n");
	if(valuePtr == nullptr || dstTensor == nullptr) {
		MIOPEN_THROW(miopenStatusBadParm);
	}

	// Launch kernels using the handle

	// [MD]: Can we just use host enqueue API to set the values in
	// the buffer?

	std::string program_name; // CL kernel filename
	std::string kernel_name; // kernel name
	std::string parms; // kernel parameters

//	OCLKernel kernel = KernelCache::get(queue, program_name, kernel_name, parms);

}

void TensorDescriptor::ScaleTensor(Handle& /* handle */,
		Data_t							dstTensor,
		const void						* /*alpha*/) {

	printf("To be implemented (ScaleTensor) \n");
	if(dstTensor == nullptr) {
		MIOPEN_THROW(miopenStatusBadParm);
	}


	// [MD]: Can we just use the TransformTensor Kernel with beta = 0 ?

	std::string program_name; // CL kernel filename
	std::string kernel_name; // kernel name
	std::string parms; // kernel parameters

	//OCLKernel kernel = KernelCache::get(queue, program_name, kernel_name, parms);

}

// Free Tensor Functions
//
static void CreateBitmapAndGrid(unsigned int &bitmap, std::vector<int> &a_lens, std::vector<int> &c_lens, int &num_wg, int &work, int d)
{
    bitmap |= (1 << (a_lens.size() - d)); // update bitmap for first_not_one
    for(int i = (d-2); i>= 0; i--) {
        if(a_lens[i] != 1) {
            bitmap |= (1 << (a_lens.size()-(i+1))); // works only 4d tensors in NCHW
            num_wg *= a_lens[i];
        }
        else
            work *= c_lens[i];
    }
}

void AddTensor(Handle&              handle,
			const void              * /*alpha*/,
			const TensorDescriptor& aTensorDesc,
			ConstData_t             ATensor,
			const void              * /*beta*/,
			const TensorDescriptor& cTensorDesc,
			Data_t                  CTensor) {

    if(ATensor == nullptr || CTensor == nullptr) {
        MIOPEN_THROW(miopenStatusBadParm);
    }

    auto a_lens = aTensorDesc.GetLengths();
    auto c_lens = cTensorDesc.GetLengths();

    if(a_lens.size() != c_lens.size()) {
        MIOPEN_THROW("Number of Tensor dims do not match: " + std::to_string(a_lens.size()) + ", " + std::to_string(c_lens.size()));
    }

    for(auto i = 0; i < a_lens.size(); i++) {
        if(a_lens[i] != 1 && a_lens[i] != c_lens[i]) {
            MIOPEN_THROW("ATensor dim != 1 && ATensor dim != CTensor dim: " + std::to_string(i));
        }
    }

    auto first_not_one = std::find_if(a_lens.rbegin(), a_lens.rend(), [](int i){ return i != 1; });
    auto d = std::distance(a_lens.begin(), first_not_one.base());

    int num_wg = *first_not_one;
    int work_per_wg = std::accumulate(c_lens.begin() + d, c_lens.end(), 1, std::multiplies<int>());

    int c_n, c_c, c_h, c_w;
    std::tie(c_n, c_c, c_h, c_w) = tie4(cTensorDesc.GetLengths());

    int a_c, a_h, a_w;
    std::tie(std::ignore, a_c, a_h, a_w) = tie4(aTensorDesc.GetLengths());

    int c_nstride, c_cstride;
    std::tie(c_nstride, c_cstride, std::ignore, std::ignore) = tie4(cTensorDesc.GetStrides());
    
    int a_nstride, a_cstride;
    std::tie(a_nstride, a_cstride, std::ignore, std::ignore) = tie4(aTensorDesc.GetStrides());

    unsigned int bitmap = 0;
    CreateBitmapAndGrid(bitmap, a_lens, c_lens, num_wg, work_per_wg, d);

    // Forward Convolution Bias specialization
    auto fwd_conv_bias = bitmap & 4 ? 1 : 0;
    auto incr_wg = 0;
    if(fwd_conv_bias == 1
            && num_wg < 640 && work_per_wg > 256) { //640 workgroups of size 256 needed to completely fill the GPU
        work_per_wg /= c_n;
        num_wg *= c_n;
        incr_wg = 1;
    }

    std::string parms = " -DFWD_CONV_BIAS=" + std::to_string(fwd_conv_bias) +
                        " -DINCR_WG=" + std::to_string(incr_wg);

    std::string program_name = "MIOpenTensorKernels.cl";
    std::string kernel_name = "AddTensor";

	const std::vector<size_t> vld(1, 256);
	const std::vector<size_t> vgd(1, num_wg*256);

    handle.GetKernel(kernel_name,
            "",
            program_name,
            kernel_name,
            vld,
            vgd,
            parms) (ATensor, a_c, a_h, a_w, a_nstride, a_cstride, CTensor, c_n, c_c, c_h, c_w, c_nstride, c_cstride, bitmap, work_per_wg);

}

void TransformTensor(Handle& /* handle */,
			const void * /*alpha*/,
			const TensorDescriptor& srcTensorDesc,
			ConstData_t  /*srcTensor*/,
			const void * /*beta*/,
			const TensorDescriptor& destTensorDesc,
			Data_t  /*destTensor*/) {

	printf("To be implemented (TransformTensor) \n");

	if(destTensorDesc == srcTensorDesc) {
		MIOPEN_THROW(miopenStatusBadParm);
	}

	// Check that output tensors do not overlap .. output tensors cannot be transformed in place .. no aliasing
	// Implement conversion of unsupported tensor to a supported one
	// Launch kernels using the handle
	// If beta[0] = 0 then just a memcopy with scaled alpha[0]?

	std::string program_name; // CL kernel filename
	std::string kernel_name; // kernel name
	std::string parms; // kernel parameters

//	OCLKernel kernel = KernelCache::get(queue, program_name, kernel_name, parms);

	// If beta = 0, y = alpha*x
}

void OpTensor(Handle& /* handle */,
		miopenTensorOp_t				 /*tensorOp*/,
		const void						* /*alpha1*/,
		const TensorDescriptor&	inputTensorDesc1,
		ConstData_t					 /*inputTensor1*/,
		const void						* /*alpha2*/,
		const TensorDescriptor&	inputTensorDesc2,
		ConstData_t					 /*inputTensor2*/,
		const void						* /*beta*/,
		const TensorDescriptor& destTensorDesc,
		Data_t							 /*destTensor*/) {

	printf("To be implemented (Op Tensor) \n");

	// inputTensor1 and dstTensor must have same dims
	if(destTensorDesc.GetLengths() != inputTensorDesc1.GetLengths()) {
		MIOPEN_THROW(miopenStatusBadParm);
	}

	// input Tensor2 and dstTensor must have same dims or all the dims of
	// inputTensor2 must be 1
	if(
		destTensorDesc.GetLengths() != inputTensorDesc2.GetLengths() && 
		! std::all_of(inputTensorDesc2.GetLengths().begin(), inputTensorDesc2.GetLengths().end(), [](int x) { return x == 1; })
	) 
	{
		MIOPEN_THROW(miopenStatusBadParm);
	}
	
	if(destTensorDesc.GetType() != inputTensorDesc1.GetType() && destTensorDesc.GetType() != inputTensorDesc2.GetType()) {
		MIOPEN_THROW(miopenStatusBadParm);
	}

	// Launch kernels using the handle

	std::string program_name; // CL kernel filename
	std::string kernel_name; // kernel name
	std::string parms; // kernel parameters

	//OCLKernel kernel = KernelCache::get(queue, program_name, kernel_name, parms);

}

void CopyTensor(Handle &handle, 
		const TensorDescriptor &srcDesc,
		ConstData_t src,
		const TensorDescriptor &destDesc,
		Data_t dest) {

	if(srcDesc.GetElementSize() != destDesc.GetElementSize() || srcDesc.GetType() != destDesc.GetType()) {
		MIOPEN_THROW(miopenStatusBadParm);
	}
	size_t srcSize = srcDesc.GetElementSize();

	handle.Copy(src, dest, srcSize*sizeof(srcDesc.GetType()));
}

} // namespace miopen
