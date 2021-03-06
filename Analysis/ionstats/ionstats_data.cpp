/* Copyright (C) 2012 Ion Torrent Systems, Inc. All Rights Reserved */


#include "ionstats_data.h"

static const char all_nucleotides_char[] = {'A','C','G','T'};
vector<char> all_nucleotides( all_nucleotides_char, all_nucleotides_char + sizeof(all_nucleotides_char)/sizeof(all_nucleotides_char[0]) );

void ErrorData::Initialize(unsigned int histogram_length) {
  region_origin_ = make_pair(0,0);
  region_dim_ = make_pair(0,0);
  ins_.Initialize(histogram_length);
  del_.Initialize(histogram_length);
  sub_.Initialize(histogram_length);
  align_start_.Initialize(histogram_length);
  align_stop_.Initialize(histogram_length);
}

void ErrorData::Initialize(unsigned int histogram_length, vector<unsigned int> &region_origin, vector<unsigned int> &region_dim) {
  assert(region_origin.size()==2);
  assert(region_dim.size()==2);
  region_origin_ = make_pair(region_origin[0],region_origin[1]);
  region_dim_ = make_pair(region_dim[0],region_dim[1]);
  ins_.Initialize(histogram_length);
  del_.Initialize(histogram_length);
  sub_.Initialize(histogram_length);
  align_start_.Initialize(histogram_length);
  align_stop_.Initialize(histogram_length);
}

void ErrorData::Initialize(vector<unsigned int> &region_origin, vector<unsigned int> &region_dim, vector<unsigned int> &error_data_dim, vector<uint64_t> &error_data) {
  assert(region_origin.size()==2);
  assert(region_dim.size()==2);
  assert(error_data_dim.size()==2);
  assert(error_data_dim[0] == ERROR_DATA_N_ROWS);
  assert(error_data.size()==error_data_dim[0]*error_data_dim[1]);
  region_origin_ = make_pair(region_origin[0],region_origin[1]);
  region_dim_ = make_pair(region_dim[0],region_dim[1]);
  ins_.Initialize(        error_data.begin()                    ,error_data.begin()+error_data_dim[1]  );
  del_.Initialize(        error_data.begin()+error_data_dim[1]*1,error_data.begin()+error_data_dim[1]*2);
  sub_.Initialize(        error_data.begin()+error_data_dim[1]*2,error_data.begin()+error_data_dim[1]*3);
  align_start_.Initialize(error_data.begin()+error_data_dim[1]*3,error_data.begin()+error_data_dim[1]*4);
  align_stop_.Initialize( error_data.begin()+error_data_dim[1]*4,error_data.begin()+error_data_dim[1]*5);
}

void ErrorData::Add(ReadAlignmentErrors &e) {
  align_start_.Add(e.first());
  align_stop_.Add(e.last());
  const vector<uint16_t> &ins_pos = e.ins();
  const vector<uint16_t> &del_pos = e.del();
  const vector<uint16_t> &del_len = e.del_len();
  const vector<uint16_t> &sub_pos = e.sub();
  for(unsigned int i=0; i<ins_pos.size(); ++i)
    ins_.Add(ins_pos[i]);
  for(unsigned int i=0; i<del_pos.size(); ++i)
    del_.Add(del_pos[i],del_len[i]);
  for(unsigned int i=0; i<sub_pos.size(); ++i)
    sub_.Add(sub_pos[i]);
}

void ErrorData::MergeFrom(ErrorData &other) {
  ins_.MergeFrom(other.ins_);
  del_.MergeFrom(other.del_);
  sub_.MergeFrom(other.sub_);
  align_start_.MergeFrom(other.align_start_);
  align_stop_.MergeFrom(other.align_stop_);
}

void ErrorData::writeH5(hid_t &file_id, string group_name) {
  herr_t status;

  hid_t group_id = H5CreateOrOpenGroup(file_id, group_name);
  hid_t dataset_id;
  hid_t dataspace_id;

  vector<unsigned int> buf32;
  // Write the region origin & dimensions
  buf32.resize(2);
  hsize_t  region_dims[1];
  // origin
  buf32[0] = region_origin_.first;
  buf32[1] = region_origin_.second;
  region_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, region_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "region_origin", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf32[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);
  // dim
  buf32[0] = region_dim_.first;
  buf32[1] = region_dim_.second;
  region_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, region_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "region_dim", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf32[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // error_data_dim
  unsigned int n_row=5;
  unsigned int n_col=ins_.Size();
  buf32[0] = n_row;
  buf32[1] = n_col;
  region_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, region_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "error_data_dim", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf32[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // make buffer for Nx5 matrix of error data to write
  vector<uint64_t> buf64;
  LoadErrorDataBuffer(n_col,n_row,buf64);

  // Create compressed dataset 
  hsize_t  dims[2];
  dims[0] = n_row;
  dims[1] = n_col;
  dataspace_id = H5Screate_simple (2, dims, NULL);
  hid_t plist_id  = H5Pcreate (H5P_DATASET_CREATE);
  hsize_t  cdims[2];
  cdims[0] = min(n_row,(unsigned int) 20);
  cdims[1] = min(n_col,(unsigned int) 100);
  status = H5Pset_chunk (plist_id, 2, cdims);
  status = H5Pset_deflate (plist_id, 9); 

  dataset_id = H5Dcreate2 (group_id, "error_data", H5T_NATIVE_UINT_LEAST64, dataspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf64[0]);

  status = H5Dclose (dataset_id);
  status = H5Pclose (plist_id);
  status = H5Sclose (dataspace_id);
  status = H5Gclose (group_id);
}

void ErrorData::LoadErrorDataBuffer(unsigned int n_col, unsigned int n_row, vector<uint64_t> &buf) {
  buf.clear();
  buf.reserve(n_col*n_row);
  for(unsigned int i=0; i<n_col; ++i)
    buf.push_back(ins_.Count(i));
  for(unsigned int i=0; i<n_col; ++i)
    buf.push_back(del_.Count(i));
  for(unsigned int i=0; i<n_col; ++i)
    buf.push_back(sub_.Count(i));
  for(unsigned int i=0; i<n_col; ++i)
    buf.push_back(align_start_.Count(i));
  for(unsigned int i=0; i<n_col; ++i)
    buf.push_back(align_stop_.Count(i));
}




int ErrorData::readH5(hid_t group_id) {

  // Read all the data
  hid_t dataset_id;
  vector<unsigned int> region_origin(2,0);
  dataset_id = H5Dopen2(group_id,"region_origin",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &region_origin[0]);
  H5Dclose(dataset_id);
  vector<unsigned int> region_dim(2,0);
  dataset_id = H5Dopen2(group_id,"region_dim",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &region_dim[0]);
  H5Dclose(dataset_id);
  vector<unsigned int> error_data_dim(2,0);
  dataset_id = H5Dopen2(group_id,"error_data_dim",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &error_data_dim[0]);
  H5Dclose(dataset_id);
  if(error_data_dim[0] != ERROR_DATA_N_ROWS)
    return(EXIT_FAILURE);

  vector<uint64_t> error_data(error_data_dim[0]*error_data_dim[1],0);
  dataset_id = H5Dopen2(group_id,"error_data",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &error_data[0]);
  H5Dclose(dataset_id);

  // Store data in object
  Initialize(region_origin,region_dim,error_data_dim,error_data);

  return(EXIT_SUCCESS);
}


void ErrorData::SaveToJson(Json::Value& json_value) {
  ins_.SaveToJson(json_value["ins"]);
  del_.SaveToJson(json_value["del"]);
  sub_.SaveToJson(json_value["sub"]);
  align_start_.SaveToJson(json_value["align_start"]);
  align_stop_.SaveToJson(json_value["align_stop"]);
}

void ErrorData::MergeFrom(Json::Value& json_value, bool &found) {
  SimpleHistogram temp;
  if(json_value.isMember("ins")) {
    temp.LoadFromJson(json_value["ins"]);
    ins_.MergeFrom(temp);
    found = true;
  }
  if(json_value.isMember("del")) {
    temp.LoadFromJson(json_value["del"]);
    del_.MergeFrom(temp);
    found = true;
  }
  if(json_value.isMember("sub")) {
    temp.LoadFromJson(json_value["sub"]);
    sub_.MergeFrom(temp);
    found = true;
  }
  if(json_value.isMember("align_start")) {
    temp.LoadFromJson(json_value["align_start"]);
    align_start_.MergeFrom(temp);
    found = true;
  }
  if(json_value.isMember("align_stop")) {
    temp.LoadFromJson(json_value["align_stop"]);
    align_stop_.MergeFrom(temp);
    found = true;
  }
}

int HpData::readH5(hid_t group_id) {

  // Read all the data
  hid_t dataset_id;
  vector<unsigned int> region_origin(2,0);
  dataset_id = H5Dopen2(group_id,"region_origin",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &region_origin[0]);
  H5Dclose(dataset_id);
  vector<unsigned int> region_dim(2,0);
  dataset_id = H5Dopen2(group_id,"region_dim",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &region_dim[0]);
  H5Dclose(dataset_id);
  unsigned int hp_data_dim=0;
  dataset_id = H5Dopen2(group_id,"hp_data_dim",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &hp_data_dim);
  H5Dclose(dataset_id);
  if(hp_data_dim==0)
    return(EXIT_FAILURE);

  Initialize(hp_data_dim-1,region_origin,region_dim);

  vector<uint64_t> hp_data(hp_data_dim*hp_data_dim,0);
  for(vector<char>::iterator it=all_nucleotides.begin(); it != all_nucleotides.end(); ++it) {
    string this_nuc(it,it+1);
    if(H5Lexists(group_id, this_nuc.c_str(), H5P_DEFAULT)) {
      dataset_id = H5Dopen2(group_id,this_nuc.c_str(),H5P_DEFAULT);
      H5Dread(dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &hp_data[0]);
      H5Dclose(dataset_id);
      hp_count_[*it].resize(hp_data_dim);
      uint64_t *begin = &hp_data[0];
      for(unsigned int i=0; i<hp_data_dim; ++i) {
        uint64_t *end = begin+hp_data_dim;
        hp_count_[*it][i].assign(begin, end);
        begin = end;
      }
    }
  }

  return(EXIT_SUCCESS);
}


void HpData::Initialize(unsigned int max_hp, vector<unsigned int> &o, vector<unsigned int> &d) {
  if(o.size()==2)
    origin_ = make_pair(o[0],o[1]);
  else
    origin_ = make_pair(0,0);
  if(d.size()==2)
    dim_ = make_pair(d[0],d[1]);
  else
    dim_ = make_pair(0,0);
  Initialize(max_hp);
}

void HpData::Initialize(unsigned int max_hp) {
  max_hp_ = max_hp;
  for(vector<char>::iterator it = all_nucleotides.begin(); it!=all_nucleotides.end(); ++it) {
    hp_count_[*it].resize(1+max_hp);
    for(unsigned int i=0; i<=max_hp; ++i)
      hp_count_[*it][i].assign(1+max_hp,0);
  }
}

void HpData::Add(vector<char> &ref_hp_nuc, vector<uint16_t> &ref_hp_len, vector<int16_t> &ref_hp_err) {
  unsigned int n_hp = ref_hp_nuc.size();
  for(unsigned int i=0; i<n_hp; ++i) {
    if(ref_hp_len[i] > max_hp_)
      continue;
    int read_hp_len = ref_hp_len[i] + ref_hp_err[i];
    if(read_hp_len > (int) max_hp_)
      continue;
assert(read_hp_len >= 0);
    hp_count_[ref_hp_nuc[i]][ref_hp_len[i]][read_hp_len] += 1;
  }
}


void HpData::LoadHpDataBuffer(unsigned int n_col, unsigned int n_row, vector<uint64_t> &buf, vector< vector<uint64_t> > &hp_table) {
  buf.resize(n_row*n_col);
  vector<uint64_t>::iterator it = buf.begin();
  for(unsigned int i=0; i<n_row; ++i) {
    assert(hp_table[i].size()==(unsigned int)n_col);
    for(unsigned int j=0; j<n_col; ++j,++it)
      *it = hp_table[i][j];
  }
}


void HpData::writeH5(hid_t &file_id, string group_name) {
  herr_t status;

  hid_t group_id = H5CreateOrOpenGroup(file_id, group_name);
  hid_t dataset_id;
  hid_t dataspace_id;

  vector<unsigned int> coord_buf(2,0);
  // Write the region origin & dimensions
  hsize_t  region_dims[1];
  // origin
  coord_buf[0] = origin_.first;
  coord_buf[1] = origin_.second;
  region_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, region_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "region_origin", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &coord_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);
  // dim
  coord_buf[0] = dim_.first;
  coord_buf[1] = dim_.second;
  region_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, region_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "region_dim", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &coord_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  unsigned int n_row = 0;
  unsigned int n_col = 0;
  if(hp_count_.begin() != hp_count_.end()) {
    n_row = hp_count_.begin()->second.size();
    n_col = (n_row > 0) ? hp_count_.begin()->second[0].size() : 0;
  }

  // dim
  coord_buf[0] = n_row;
  coord_buf[1] = n_col;
  region_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, region_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "hp_data_dim", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &coord_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // now write the per-nuc error tables
  vector<uint64_t> hp_table_buf;
  map< char, vector< vector<uint64_t> > >::iterator it;
  for(it = hp_count_.begin(); it != hp_count_.end(); ++it) {
    string nuc;
    stringstream ss;
    ss << it->first;
    ss >> nuc;
  
    LoadHpDataBuffer(n_col,n_row,hp_table_buf,it->second);
  
    // Create compressed dataset 
    hsize_t  dims[2];
    dims[0] = n_row;
    dims[1] = n_col;
    dataspace_id = H5Screate_simple (2, dims, NULL);
    hid_t plist_id  = H5Pcreate (H5P_DATASET_CREATE);
    hsize_t  cdims[2];
    cdims[0] = min(n_row,(unsigned int) 20);
    cdims[1] = min(n_col,(unsigned int) 20);
    status = H5Pset_chunk (plist_id, 2, cdims);
    status = H5Pset_deflate (plist_id, 9); 
  
    dataset_id = H5Dcreate2 (group_id, nuc.c_str(), H5T_NATIVE_UINT_LEAST64, dataspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT); 
    status = H5Dwrite (dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &hp_table_buf[0]);
  
    status = H5Dclose (dataset_id);
    status = H5Pclose (plist_id);
    status = H5Sclose (dataspace_id);
  }

  status = H5Gclose (group_id);
}


int HpData::MergeFrom(HpData &other) {
  if(max_hp_ != other.MaxHp())
    return(EXIT_FAILURE);
  for(vector<char>::iterator it=all_nucleotides.begin(); it != all_nucleotides.end(); ++it) {
    map< char, vector< vector<uint64_t> > >::iterator input=other.HpCountFind(*it);
    if(input == other.HpCountEnd())
      continue;
    map< char, vector< vector<uint64_t> > >::iterator output=hp_count_.find(*it);
    if(output == hp_count_.end()) {
      hp_count_[*it].resize(input->second.size());
      for(unsigned int i=0; i<=max_hp_; ++i)
        hp_count_[*it][i].assign(input->second[i].begin(), input->second[i].end());
    } else {
      for(unsigned int i=0; i<=max_hp_; ++i)
        for(unsigned int j=0; j<=max_hp_; ++j)
          hp_count_[*it][i][j] += input->second[i][j];
    }
  }
  return(EXIT_SUCCESS);
}


hid_t H5CreateOrOpenGroup(hid_t &file_id, string &group_name) {
  hid_t group_id;
  if(group_name == "/") {
    group_id = H5Gopen2(file_id, group_name.c_str(), H5P_DEFAULT);
  } else {
    // first make sure the base group exists
    string delim = "/";
    int pos = group_name.rfind(delim);
    if((pos != (int) std::string::npos) && (pos != 0)) {
      string subgroup = group_name.substr(0,pos);
      group_id = H5CreateOrOpenGroup(file_id, subgroup);
      H5Gclose (group_id);
    }
    // then open or create the group we want
    if(H5Lexists(file_id, group_name.c_str(), H5P_DEFAULT)) {
      group_id = H5Gopen2(file_id, group_name.c_str(), H5P_DEFAULT);
    } else {
      group_id = H5Gcreate2(file_id, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
  }
  return(group_id);
}

void RegionalSummary::Initialize(unsigned int max_hp, unsigned int n_flow, vector<unsigned int> &origin, vector<unsigned int> &dim) {
  assert(origin.size()==2);
  assert(dim.size()==2);
  origin_ = make_pair(origin[0],origin[1]);
  dim_ = make_pair(dim[0],dim[1]);
  max_hp_ = max_hp;
  n_flow_ = n_flow;
  hp_count_.resize(n_flow_);
  hp_err_.resize(n_flow_);
  for(unsigned int i=0; i<n_flow_; ++i) {
    hp_count_[i].assign(max_hp_,0);
    hp_err_[i].assign(max_hp_,0);
  }
}

void RegionalSummary::Add(ReadAlignmentErrors &e) {
  n_err_ += e.ins().size() + e.sub().size();
  const vector<uint16_t> &del_len = e.del_len();
  for(unsigned int i=0; i<del_len.size(); ++i)
    n_err_ += del_len[i];
  n_aligned_ += e.last() - e.first() + 1;
}

void RegionalSummary::Add(vector<uint16_t> &ref_hp_len, vector<int16_t> &ref_hp_err, vector<uint16_t> &ref_hp_flow) {
  unsigned int n_hp = ref_hp_len.size();
  for(unsigned int i=0; i<n_hp; ++i) {
    if(ref_hp_len[i] > max_hp_)
      continue;
    if(ref_hp_len[i] == 0)
      continue;
    assert(ref_hp_flow[i] < n_flow_);
    hp_count_[ref_hp_flow[i]][ref_hp_len[i]-1] += 1;
    if(ref_hp_err[i] != 0)
      hp_err_[ref_hp_flow[i]][ref_hp_len[i]-1] += 1;
  }
}

int RegionalSummary::MergeFrom(RegionalSummary &other) {
  if(max_hp_ != other.MaxHp())
    return(EXIT_FAILURE);
  if(n_flow_ != other.nFlow())
    return(EXIT_FAILURE);
  n_err_ += other.nErr();
  n_aligned_ += other.nAligned();
  const vector< vector<uint64_t> > &other_hp_count = other.HpCount();
  const vector< vector<uint64_t> > &other_hp_err   = other.HpErr();
  for(unsigned int iFlow=0; iFlow<n_flow_; ++iFlow) {
    for(unsigned int iHp=0; iHp<max_hp_; ++iHp) {
      hp_count_[iFlow][iHp] += other_hp_count[iFlow][iHp];
      hp_err_[iFlow][iHp]   += other_hp_err[iFlow][iHp];
    }
  }
  return(EXIT_SUCCESS);
}

void RegionalSummary::writeH5(hid_t &file_id, string group_name) {
  herr_t status;

  hid_t group_id = H5CreateOrOpenGroup(file_id, group_name);
  hid_t dataset_id;
  hid_t dataspace_id;

  hsize_t  h5_dims[1];

  // region_origin
  vector<unsigned int> coord_buf(2,0);
  coord_buf[0] = origin_.first;
  coord_buf[1] = origin_.second;
  h5_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, h5_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "region_origin", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &coord_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // region_dim
  coord_buf[0] = dim_.first;
  coord_buf[1] = dim_.second;
  h5_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, h5_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "region_dim", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &coord_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // n_err_
  h5_dims[0] = 1;
  dataspace_id = H5Screate_simple (1, h5_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "n_err", H5T_NATIVE_UINT_LEAST64, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &n_err_);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // n_aligned_
  h5_dims[0] = 1;
  dataspace_id = H5Screate_simple (1, h5_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "n_aligned", H5T_NATIVE_UINT_LEAST64, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &n_aligned_);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // data_dim
  AssertDims();
  coord_buf[0] = n_flow_;
  coord_buf[1] = max_hp_;
  h5_dims[0] = 2;
  dataspace_id = H5Screate_simple (1, h5_dims, NULL);
  dataset_id = H5Dcreate2 (group_id, "data_dim", H5T_NATIVE_UINT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &coord_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Sclose (dataspace_id);

  // hp_count_ and hp_err_
  hsize_t  dims[2];
  dims[0] = n_flow_;
  dims[1] = max_hp_;
  vector<uint64_t> data_buf;
  hsize_t  cdims[2];
  hid_t plist_id;

  // hp_count_
  LoadDataBuffer(max_hp_,n_flow_,data_buf,hp_count_);
  dataspace_id = H5Screate_simple (2, dims, NULL);
  plist_id  = H5Pcreate (H5P_DATASET_CREATE);
  cdims[0] = min(n_flow_,(unsigned int) 20);
  cdims[1] = min(max_hp_,(unsigned int) 20);
  status = H5Pset_chunk (plist_id, 2, cdims);
  status = H5Pset_deflate (plist_id, 9); 
  dataset_id = H5Dcreate2 (group_id, "hp_count", H5T_NATIVE_UINT_LEAST64, dataspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Pclose (plist_id);
  status = H5Sclose (dataspace_id);

  // hp_err_
  LoadDataBuffer(max_hp_,n_flow_,data_buf,hp_err_);
  dataspace_id = H5Screate_simple (2, dims, NULL);
  plist_id  = H5Pcreate (H5P_DATASET_CREATE);
  cdims[0] = min(n_flow_,(unsigned int) 20);
  cdims[1] = min(max_hp_,(unsigned int) 20);
  status = H5Pset_chunk (plist_id, 2, cdims);
  status = H5Pset_deflate (plist_id, 9); 
  dataset_id = H5Dcreate2 (group_id, "hp_err", H5T_NATIVE_UINT_LEAST64, dataspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT); 
  status = H5Dwrite (dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data_buf[0]);
  status = H5Dclose (dataset_id);
  status = H5Pclose (plist_id);
  status = H5Sclose (dataspace_id);

  status = H5Gclose (group_id);
}

int RegionalSummary::LoadDataBuffer(unsigned int n_col, unsigned int n_row, vector<uint64_t> &buf, const vector< vector<uint64_t> > & data) {
  assert(data.size()==n_row);
  if(n_row > 0)
    assert(data[0].size()==n_col);
  buf.resize(n_row*n_col);
  vector<uint64_t>::iterator it = buf.begin();
  for(unsigned int i=0; i<n_row; ++i)
    for(unsigned int j=0; j<n_col; ++j,++it)
      *it = data[i][j];
  return(EXIT_SUCCESS);
}

void RegionalSummary::AssertDims(void) {
  assert(hp_count_.size() == n_flow_);
  assert(hp_err_.size() == n_flow_);
  if(n_flow_ > 0) {
    assert(hp_count_[0].size() == max_hp_);
    assert(hp_err_[0].size() == max_hp_);
  }
}

int RegionalSummary::readH5(hid_t group_id) {
  // Read all the data
  hid_t dataset_id;
  vector<unsigned int> region_origin(2,0);
  dataset_id = H5Dopen2(group_id,"region_origin",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &region_origin[0]);
  H5Dclose (dataset_id);
  vector<unsigned int> region_dim(2,0);
  dataset_id = H5Dopen2(group_id,"region_dim",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &region_dim[0]);
  H5Dclose (dataset_id);
  vector<unsigned int> data_dim(2,0);
  dataset_id = H5Dopen2(group_id,"data_dim",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data_dim[0]);
  H5Dclose (dataset_id);
  Initialize(data_dim[1],data_dim[0],region_origin,region_dim);

  dataset_id = H5Dopen2(group_id,"n_err",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &n_err_);
  H5Dclose (dataset_id);
  dataset_id = H5Dopen2(group_id,"n_aligned",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &n_aligned_);
  H5Dclose (dataset_id);
  unsigned int n_data = data_dim[0] * data_dim[1];
  vector<uint64_t> hp_count(n_data,0);
  dataset_id = H5Dopen2(group_id,"hp_count",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &hp_count[0]);
  H5Dclose (dataset_id);
  vector<uint64_t> hp_err(n_data,0);
  dataset_id = H5Dopen2(group_id,"hp_err",H5P_DEFAULT);
  H5Dread(dataset_id, H5T_NATIVE_UINT_LEAST64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &hp_err[0]);
  H5Dclose (dataset_id);
  uint64_t *countStart = &hp_count[0];
  uint64_t *errStart   = &hp_err[0];
  for(unsigned int i_flow=0; i_flow <n_flow_; ++i_flow) {
    uint64_t *countEnd = countStart + max_hp_;
    uint64_t *errEnd   = errStart   + max_hp_;
    hp_count_[i_flow].assign(countStart, countEnd);
    hp_err_[i_flow].assign(errStart, errEnd);
    countStart = countEnd;
    errStart = errEnd;
  }

  return(EXIT_SUCCESS);
}

