// Copyright (c) 2010, Roman Khmelichek
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Roman Khmelichek nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//==============================================================================================================================================================
// Author(s): Roman Khmelichek
//
//==============================================================================================================================================================

#include "document_collection.h"

#include <cstdio>
#include <cstdlib>

#include <fstream>

#include "config_file_properties.h"
#include "configuration.h"
#include "globals.h"
#include "logger.h"
#include "posting_collection.h"
#include "uncompress_file.h"
using namespace std;

/**************************************************************************************************************************************************************
 * Document
 *
 **************************************************************************************************************************************************************/
Document::Document(const char* doc_buf, int doc_len, const char* url_buf, int url_len, uint32_t doc_id) :
  doc_buf_(doc_buf), doc_len_(doc_len), url_buf_(url_buf), url_len_(url_len), doc_id_(doc_id) {

}

/**************************************************************************************************************************************************************
 * DocumentCollection
 *
 **************************************************************************************************************************************************************/
DocumentCollection::DocumentCollection(const string& file_path) :
  processed_(false), file_path_(file_path), lang_(DocumentCollection::ENGLISH), initial_doc_id_(0), final_doc_id_(0) {
}

int DocumentCollection::Fill(char** document_collection_buf, int* document_collection_buf_size) {
  int document_collection_buf_len;
  UncompressFile(file_path_.c_str(), document_collection_buf, document_collection_buf_size, &document_collection_buf_len);
  return document_collection_buf_len;
}

/**************************************************************************************************************************************************************
 * IndexCollection
 *
 **************************************************************************************************************************************************************/
void IndexCollection::AddDocumentCollection(const string& path) {
  ifstream ifs;
  ifs.open(path.c_str(), ifstream::in);
  ifs.close();
  if (!ifs.fail()) {
    DocumentCollection doc_collection(path);
    doc_collections_.push_back(doc_collection);
  } else {
    GetErrorLogger().Log("Could not open document collection file '" + path + "'. Skipping...", false);
  }
}

void IndexCollection::ProcessDocumentCollections(istream& is) {
  string path;
  while (getline(is, path)) {
    if (path.size() > 0)
      AddDocumentCollection(path);
  }
}

/**************************************************************************************************************************************************************
 * CollectionIndexer
 *
 **************************************************************************************************************************************************************/
CollectionIndexer::CollectionIndexer() :
  document_collection_buffer_size_(atol(Configuration::GetConfiguration().GetValue(config_properties::kDocumentCollectionBufferSize).c_str())),
      document_collection_buffer_(new char[document_collection_buffer_size_]), parser_callback_(&GetPostingCollectionController()),
      parser_(Parser<IndexingParserCallback>::kManyDoc, GetAndVerifyDocType(), &parser_callback_), doc_id_(0), avg_doc_length_(0) {
  if (document_collection_buffer_size_ == 0)
    GetErrorLogger().Log("Check configuration setting for '" + string(config_properties::kDocumentCollectionBufferSize) + "'.", true);
}

CollectionIndexer::~CollectionIndexer() {
  delete[] document_collection_buffer_;
}

Parser<IndexingParserCallback>::DocType CollectionIndexer::GetAndVerifyDocType() {
  string document_collection_format =
      Configuration::GetResultValue(Configuration::GetConfiguration().GetStringValue(config_properties::kDocumentCollectionFormat));

  Parser<IndexingParserCallback>::DocType doc_type = Parser<IndexingParserCallback>::GetDocumentCollectionFormat(document_collection_format.c_str());
  if (doc_type == Parser<IndexingParserCallback>::kNoSuchDocType) {
    Configuration::ErroneousValue(config_properties::kDocumentCollectionFormat, document_collection_format);
  }

  return doc_type;
}

void CollectionIndexer::ParseTrec() {
  int total_num_docs_found = 0;
  for (vector<DocumentCollection>::iterator i = doc_collections_.begin(); i != doc_collections_.end(); ++i) {
    GetDefaultLogger().Log("Processing: " + i->file_path(), false);

    int document_collection_buffer_len = i->Fill(&document_collection_buffer_, &document_collection_buffer_size_);

    i->set_initial_doc_id(doc_id_);
    int num_docs_parsed = parser_.ParseDocumentCollection(document_collection_buffer_, document_collection_buffer_len, doc_id_, avg_doc_length_);
    i->set_processed(true);
    GetDefaultLogger().Log("Found: " + Stringify(num_docs_parsed) + " documents.", false);
    i->set_final_doc_id(doc_id_ - 1);

    total_num_docs_found += num_docs_parsed;
  }

  GetDefaultLogger().Log("Total number of documents found: " + Stringify(total_num_docs_found), false);

  GetPostingCollectionController().Finish();
}

void CollectionIndexer::OutputDocumentCollectionDocIdRanges(const char* filename) {
  ofstream document_collections_doc_id_ranges_stream(filename);
  if (!document_collections_doc_id_ranges_stream) {
    GetErrorLogger().Log("Could not open '" + string(filename) + "' for writing.", true);
  }

  document_collections_doc_id_ranges_stream << "'Document Collection Filename'" << "\t" << "'Initial DocID'" << "\t" << "'Final DocID'" << "\n";
  for (vector<DocumentCollection>::iterator i = doc_collections_.begin(); i != doc_collections_.end(); ++i) {
    if (i->processed())
      document_collections_doc_id_ranges_stream << i->file_path() << "\t" << i->initial_doc_id() << "\t" << i->final_doc_id() << "\n";
  }
  document_collections_doc_id_ranges_stream.close();
}

/**************************************************************************************************************************************************************
 * CollectionUrlExtractor
 *
 **************************************************************************************************************************************************************/
CollectionUrlExtractor::CollectionUrlExtractor() :
  document_collection_buffer_size_(atol(Configuration::GetConfiguration().GetValue(config_properties::kDocumentCollectionBufferSize).c_str())),
      document_collection_buffer_(new char[document_collection_buffer_size_]),
      parser_(Parser<DocUrlRetrievalParserCallback>::kManyDoc, GetAndVerifyDocType(), &parser_callback_), doc_id_(0), avg_doc_length_(0) {
  if (document_collection_buffer_size_ == 0)
    GetErrorLogger().Log("Check configuration setting for '" + string(config_properties::kDocumentCollectionBufferSize) + "'.", true);
}

CollectionUrlExtractor::~CollectionUrlExtractor() {
  delete[] document_collection_buffer_;
}

Parser<DocUrlRetrievalParserCallback>::DocType CollectionUrlExtractor::GetAndVerifyDocType() {
  string document_collection_format =
      Configuration::GetResultValue(Configuration::GetConfiguration().GetStringValue(config_properties::kDocumentCollectionFormat));

  Parser<DocUrlRetrievalParserCallback>::DocType doc_type = Parser<DocUrlRetrievalParserCallback>::GetDocumentCollectionFormat(document_collection_format.c_str());
  if (doc_type == Parser<DocUrlRetrievalParserCallback>::kNoSuchDocType) {
    Configuration::ErroneousValue(config_properties::kDocumentCollectionFormat, document_collection_format);
  }

  return doc_type;
}

void CollectionUrlExtractor::ParseTrec(const char* document_urls_filename) {
  int total_num_docs_found = 0;
  for (vector<DocumentCollection>::iterator i = doc_collections_.begin(); i != doc_collections_.end(); ++i) {
    GetDefaultLogger().Log("Processing: " + i->file_path(), false);

    int document_collection_buffer_len = i->Fill(&document_collection_buffer_, &document_collection_buffer_size_);

    i->set_initial_doc_id(doc_id_);
    int num_docs_parsed = parser_.ParseDocumentCollection(document_collection_buffer_, document_collection_buffer_len, doc_id_, avg_doc_length_);
    i->set_processed(true);
    GetDefaultLogger().Log("Found: " + Stringify(num_docs_parsed) + " documents.", false);
    i->set_final_doc_id(doc_id_ - 1);

    total_num_docs_found += num_docs_parsed;
  }

  GetDefaultLogger().Log("Total number of documents found: " + Stringify(total_num_docs_found), false);

  // Sort the URL and docID pairs.
  sort(parser_callback_.document_urls().begin(), parser_callback_.document_urls().end());

  // Write the new mapped docID, original docID, and URL of the sorted URL and docID pairs to a file.
  ofstream document_urls_stream(document_urls_filename);
  if (!document_urls_stream) {
    GetErrorLogger().Log("Could not open '" + string("document_urls") + "' for writing.", true);
  }

  uint32_t mapped_doc_id = 0;
  for (std::vector<std::pair<std::string, uint32_t> >::iterator i = parser_callback_.document_urls().begin(); i != parser_callback_.document_urls().end(); ++i) {
    document_urls_stream << mapped_doc_id << " " << i->second << " " << i->first << "\n";
    ++mapped_doc_id;
  }
  document_urls_stream.close();
}
