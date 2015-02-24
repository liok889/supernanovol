
#ifndef _ARRAY3_H
#define _ARRAY3_H 1

#include "VectorT.hxx"
#include <assert.h>

#ifndef lerpf
#define lerpf(t, v0, v1) (v0 + (v1 - v0) * t)
#endif

struct Array3Base
{
  string filebase;
  string type;
  vec3i dims;
  vec2f range;
  vec2f old_range;
  string xyz_filebase;
  vec3f ws2gs_scale;
  vec3f ws2gs_translate;
  
  void* minmax_macrocells;
  
  Array3Base(){ minmax_macrocells = 0; }
  virtual ~Array3Base(){}
  
  virtual void allocate(){}
  virtual void resize(int, int, int){}
  virtual size_t get_data_size(){ return 0; }
  virtual void read_raw(){}
  virtual void write(){}
  virtual void write_raw(){}
  virtual void write_nhdr(){}
  
  virtual void* get_data_ptr(){}
  virtual void compute_minmax(){}
  virtual float get_data_generic(int i, int j, int k) const{}
  virtual void set_data_generic(int i, int j, int k, float f){}
};

template<typename T>
struct Array3 : public Array3Base
{
  T* data;

  Array3(Array3Base* base)
  {
    filebase = base->filebase;
    type = base->type;
    dims = base->dims;
    range = base->range;
    old_range = base->old_range;
    xyz_filebase = base->xyz_filebase;
    ws2gs_translate = base->ws2gs_translate;
    ws2gs_scale = base->ws2gs_scale;
    allocate();
  }
  Array3();
  Array3(int, int, int);
	Array3(const vec3i& dimensions);
  ~Array3();

  void allocate();
  void resize(int, int, int);
  void initialize(const T);
  float trilerp(const vec3& p);
  
  void compute_minmax();
  float get_data_generic(int i, int j, int k) const;
  void set_data_generic(int i, int j, int k, float f);
  
  inline bool exists() const
  {
    return (dims[0] > 0 && dims[1] > 0 && dims[2] > 0);
  }

  void* get_data_ptr() 
  {
    return &data[0];
  }
  
  inline size_t get_data_size() 
  {
    return dims[0]*dims[1]*((size_t)dims[2])*sizeof(T);
  }

  inline T& get_data(int i, int j, int k) const
  {
		//return data[dims[1]*dims[2]*i + dims[2]*j + k];
		return data[i + dims[0]*(j + dims[1] * k)];
  }
  
  inline T get_data_safe(int i, int j, int k) const
  {
    assert(i >= 0 && j >= 0 && k >= 0 && i < dims[0] && j < dims[1] && k < dims[2]);
    T datum = {0};
    if (i >= 0 && j >= 0 && k >= 0 && i < dims[0] && j < dims[1] && k < dims[2])
      datum = data[i + dims[0]*(j + dims[1] * k)];
    return datum;
  }

  inline T* get_data_safe_p(int i, int j, int k) const
  {
    T* ptr = 0;
    if (i >= 0 && j >= 0 && k >= 0 && i < dims[2] && j < dims[1] && k < dims[0])
      ptr = &data[i + dims[0]*(j + dims[1] * k)];
    return ptr;
  }

	inline void set_data_safe(int i, int j, int k, const T& val)
	{
    assert(i >= 0 && j >= 0 && k >= 0 && i < dims[0] && j < dims[1] && k < dims[2]);
		if (i >= 0 && j >= 0 && k >= 0 && i < dims[0] && j < dims[1] && k < dims[2])
			data[i + dims[0]*(j + dims[1] * k)] =  val;
	}


	inline VectorT<double,3> gradient(int x, int y, int z) const
	{
		VectorT<double,3> gradient(0,0,0);

		if (x == 0) 
			gradient[0] = (get_data_safe(x+1, y , z ) - get_data_safe(x , y , z ));
		else if (x == dims[0] - 1)
			gradient[0] = (get_data_safe(x , y , z ) - get_data_safe(x-1, y , z ));
		else 
			gradient[0] = (get_data_safe(x+1, y , z ) - get_data_safe(x-1, y , z )) * 0.5;

		if (y == 0) 
			gradient[1] = (get_data_safe(x , y+1, z ) - get_data_safe(x , y , z ));
		else if (y ==  dims[1] - 1) 
			gradient[1] = (get_data_safe(x , y , z ) - get_data_safe(x , y-1, z ));
		else 
			gradient[1] = (get_data_safe(x , y+1, z ) - get_data_safe(x , y-1, z )) * 0.5;

		if (z == 0) 
			gradient[2] = (get_data_safe(x , y , z+1) - get_data_safe(x , y , z ));
		else 
			if (z ==  dims[2] - 1)
			gradient[2] = (get_data_safe(x , y , z ) - get_data_safe(x , y , z-1));
		else 
			gradient[2] = (get_data_safe(x , y , z+1) - get_data_safe(x , y , z-1)) * 0.5;

		return gradient;
	}

	inline VectorT<double,3> gradient_fd(int x, int y, int z) const
	{
		VectorT<double,3> gradient(0,0,0);
		
		if (x == dims[0] - 1)
			gradient[0] = (get_data_safe(x , y , z ) - get_data_safe(x-1, y , z ));
		else 
			gradient[0] = (get_data_safe(x+1, y , z ) - get_data_safe(x , y , z ));

					
		if (y ==  dims[1] - 1) 
			gradient[1] = (get_data_safe(x , y , z ) - get_data_safe(x , y-1, z ));
		else 
			gradient[1] = (get_data_safe(x , y+1, z ) - get_data_safe(x , y , z ));

		if (z ==  dims[2] - 1)
			gradient[2] = (get_data_safe(x , y , z ) - get_data_safe(x , y , z-1));
		else 
			gradient[2] = (get_data_safe(x , y , z+1) - get_data_safe(x , y , z ));

		return gradient;
	}


  void read_raw()
  {
    allocate();
    
    string filename = filebase;
    filename.append(".raw");
    
    cerr << "Reading from raw file: " << filename << endl;
    FILE* din = fopen(filename.c_str(), "rb");
    if (!din) 
    {
      cerr << "Error opening raw file: " << filename << '\n';
      exit(1);
    }
        
    size_t bytes = fread((char*)data, 1, dims[0] * dims[1] * dims[2] * sizeof(T), din);
    
    cerr << "done, bytes read = " << bytes << endl;
    
    fclose(din); 
  }
  
  void write()
  {
    write_nhdr();
    write_raw();
  }
  
  void write_nhdr()
  { 
    
    cerr << "Array3: writing " << filebase << ".nhdr" << endl;
    string filename = filebase;
    
    string file_no_path = filebase + ".raw";
    int pos = filename.find_last_of("/");
    if (pos != string::npos)
    {
      file_no_path = filebase.substr(pos) + ".raw";
    }
    
    filename.append(".nhdr");
    
    FILE* out = fopen(filename.c_str(), "w");
    
    if (!out){
      cerr << "Array3: Error opening header: " << filename << endl;
      exit(1);
    }
    
    fprintf(out, "NRRD0001\n");
    fprintf(out, "content: unknown\n");
    fprintf(out, "dimension: 3\n");
    fprintf(out, "type: %s\n", type.c_str());
    fprintf(out, "sizes: %d %d %d\n", dims[0], dims[1], dims[2]);
    fprintf(out, "min: %f\n", range[0]);
    fprintf(out, "max: %f\n", range[1]);
    fprintf(out, "old min: %f\n", old_range[0]);
    fprintf(out, "old max: %f\n", old_range[1]);
    fprintf(out, "data file: ./%s\n", string(filebase + ".raw").c_str());
    if (ws2gs_scale[0])
    {
      fprintf(out, "#xyz file: ./%s\n", string(xyz_filebase + ".xyz").c_str());
      fprintf(out, "#ws2gs_translate: %f %f %f\n", ws2gs_translate[0], ws2gs_translate[1], ws2gs_translate[2]);
      fprintf(out, "#ws2gs_scale: %f %f %f\n", ws2gs_scale[0], ws2gs_scale[1], ws2gs_scale[2]);
    }
    fclose(out);
  }

  void write_raw()
  {
    string filename = filebase;
    filename.append(".raw");
    
    cerr << "Attempting to write to raw file: " << filename << endl;
    FILE* dout = fopen(filename.c_str(), "wb");
    if (!dout) 
    {
      cerr << "Error opening raw file: " << filename << '\n';
      exit(1);
    }
    
    size_t bytes = fwrite((char*)data, 1, dims[0] * dims[1] * dims[2] * sizeof(T), dout);
    
    cerr << "done, bytes written = " << bytes << endl;

    fclose(dout);
  }

};

template<class T>
		Array3<T>::Array3()
{
	data=0;
	ws2gs_scale = vec3(0,0,0);
	range = vec2(0,0);
	old_range = vec2(0,0);
  dims = vec3i(0,0,0);
}

template<class T>
		Array3<T>::~Array3()
{
  if (data)
    delete[] data;
  if (minmax_macrocells)
    delete[] (Array3<vec2uc>*)minmax_macrocells;
}

template<class T>
void Array3<T>::allocate()
{
	if(dims[0]==0 || dims[1]==0 || dims[2]==0){
		data=0;
		return;
	}
	data=new T[dims[0] * dims[1] * dims[2]];
}

template<class T>
void Array3<T>::resize(int d1, int d2, int d3)
{
	if(data && dims[0]==d1 && dims[1]==d2 && dims[2]==d3)return;
	dims[0]=d1;
	dims[1]=d2;
	dims[2]=d3;
	if(data)
  {
    delete[] data;
	}
	allocate();
}

template<class T>
Array3<T>::Array3(int dx, int dy, int dz)
{
  dims[0] = dx;
  dims[1] = dy;
  dims[2] = dz;
	allocate();
}
template<class T>
Array3<T>::Array3(const vec3i& dimensions)
{
  dims = dimensions;
	allocate();
}

template<class T>
void Array3<T>::initialize(const T t)
{
	for(int i=0;i<dims[0];i++){
		for(int j=0;j<dims[1];j++){
			for(int k=0;k<dims[2];k++){
				data[dims[1]*dims[2]*i + dims[2]*j + k]=t;
			}
		}
	}
}


template<class T>
void Array3<T>::compute_minmax()
{
}


template<>
void Array3<float>::compute_minmax()
{
  for(int i=0; i<dims[0] * dims[1] * dims[2]; i++)
  {
    range[0] = min(range[0], data[i]);
    range[1] = max(range[1], data[i]);
  }
}

template<>
void Array3<unsigned char>::compute_minmax()
{
  unsigned char lo = 255;
  unsigned char hi = 0;
  for(int i=0; i<dims[0] * dims[1] * dims[2]; i++)
  {
    lo = min(lo, data[i]);
    hi = max(hi, data[i]);
  }
  range[0] = float(lo);
  range[1] = float(hi);
}

template<class T>
float Array3<T>::get_data_generic(int i, int j, int k) const
{
}

template<class T>
void Array3<T>::set_data_generic(int i, int j, int k, float val)
{
}


template<>
float Array3<unsigned char>::get_data_generic(int i, int j, int k) const
{
  return (float)(data[i + dims[0]*(j + dims[1] * k)]);
}

template<>
void Array3<unsigned char>::set_data_generic(int i, int j, int k, float val)
{
  data[i + dims[0]*(j + dims[1] * k)] = (unsigned char)(val);
}

template<>
float Array3<float>::get_data_generic(int i, int j, int k) const
{
  return data[i + dims[0]*(j + dims[1] * k)];
}

template<>
void Array3<float>::set_data_generic(int i, int j, int k, float val)
{
  data[i + dims[0]*(j + dims[1] * k)] = val;
}


template<typename T>
float Array3<T>::trilerp(const vec3& p)
{
  //cerr << "p = " << p << endl;
  
  vec3 pp = p;
  
  //assume it's periodic -- AARONBAD this might be a bad assumption
  for(int i=0; i<3; i++)
  {
    if (pp[i] >= dims[i])
      pp[i] -= dims[i];
    
    if (pp[i] < 0)
      pp[i] += dims[i];
  }
  
  vec3i ip;
  ip[0] = int(pp[0]);
  ip[1] = int(pp[1]);
  ip[2] = int(pp[2]);
  
  vec3f fip;
  fip[0] = float(ip[0]);
  fip[1] = float(ip[1]);
  fip[2] = float(ip[2]);
  
  vec3 dp = pp - fip;
  
  const int x0 = ip[0];
  const int y0 = ip[1];
  const int z0 = ip[2];
  const int x1 = ip[0] + 1;
  const int y1 = ip[1] + 1;
  const int z1 = ip[2] + 1;
  
  const float v000 = data[x0+dims[0]*(y0+dims[1]*(z0))];
  const float v001 = data[x1+dims[0]*(y0+dims[1]*(z0))];
  const float v010 = data[x0+dims[0]*(y1+dims[1]*(z0))];
  const float v011 = data[x1+dims[0]*(y1+dims[1]*(z0))];
  const float v100 = data[x0+dims[0]*(y0+dims[1]*(z1))];
  const float v101 = data[x1+dims[0]*(y0+dims[1]*(z1))];
  const float v110 = data[x0+dims[0]*(y1+dims[1]*(z1))];
  const float v111 = data[x1+dims[0]*(y1+dims[1]*(z1))];
  
  const float v00 = lerpf(dp[0],v000,v001);
  const float v01 = lerpf(dp[0],v010,v011);
  const float v10 = lerpf(dp[0],v100,v101);
  const float v11 = lerpf(dp[0],v110,v111);
  
  const float v0 = lerpf(dp[1],v00,v01);
  const float v1 = lerpf(dp[1],v10,v11);
  
  const float v = lerpf(dp[2],v0,v1);
  
  //cerr << "v = " << v << endl;
  
  return v;
}



#endif
